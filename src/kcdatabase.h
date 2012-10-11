/*
 * Copyright (c) 2000-2001 Apple Computer, Inc. All Rights Reserved.
 * 
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */


//
// kcdatabase - software database container implementation.
//
// A KeychainDatabase is a software storage container,
// implemented in cooperation by the AppleCSLDP CDSA plugin and this daemon.
//
#ifndef _H_KCDATABASE
#define _H_KCDATABASE

#include "localdatabase.h"

class KeychainDatabase;
class KeychainDbCommon;
class KeychainKey;


class DbIdentifier {
public:
	DbIdentifier(const DLDbIdentifier &id, DbBlob::Signature sig)
	: mIdent(id), mSig(sig) { }
	
	const DLDbIdentifier &dlDbIdentifier() const { return mIdent; }
	const DbBlob::Signature &signature() const { return mSig; }
	operator const DLDbIdentifier &() const { return dlDbIdentifier(); }
	operator const DbBlob::Signature &() const	{ return signature(); }
	const char *dbName() const			{ return mIdent.dbName(); }
	
	bool operator < (const DbIdentifier &id) const	// simple lexicographic
	{
		if (mIdent < id.mIdent) return true;
		if (id.mIdent < mIdent) return false;
		return mSig < id.mSig;
	}
	
	bool operator == (const DbIdentifier &id) const
	{ return mIdent == id.mIdent && mSig == id.mSig; }
	
private:
	DLDbIdentifier mIdent;
	DbBlob::Signature mSig;
};


//
// KeychainDatabase DbCommons
//
class KeychainDbCommon : public DbCommon,
	public DatabaseCryptoCore, public MachServer::Timer {
public:
	KeychainDbCommon(Session &ssn, const DbIdentifier &id);
	~KeychainDbCommon();
	
	bool unlockDb(DbBlob *blob, void **privateAclBlob = NULL);
	void lockDb(bool forSleep = false); // versatile lock primitive
	bool isLocked() const { return mIsLocked; } // lock status
	void setUnlocked();
	
	void activity();			// reset lock timeout
	
	void makeNewSecrets();

	const DbIdentifier &identifier() const {return mIdentifier; }
	const DLDbIdentifier &dlDbIdent() const { return identifier(); }
	const char *dbName() const { return dlDbIdent().dbName(); }
	
	DbBlob *encode(KeychainDatabase &db);
	
	void notify(NotificationEvent event);

	void sleepProcessing();

public:
    // debugging
    IFDUMP(void dumpNode());
	
protected:
	void action();				// timer queue action to lock keychain

public:
	DbIdentifier mIdentifier;	// database external identifier [const]
	// all following data locked with object lock
	uint32 sequence;			// change sequence number
	DBParameters mParams;		// database parameters (arbitrated copy)
	
	uint32 version;				// version stamp for change tracking
	
private:
	bool mIsLocked;				// logically locked
	bool mValidParams;			// mParams has been set
};


//
// A Database object represents an Apple CSP/DL open database (DL/DB) object.
// It maintains its protected semantic state (including keys) and provides controlled
// access.
//
class KeychainDatabase : public LocalDatabase {
	friend class KeychainDbCommon;
public:
	KeychainDatabase(const DLDbIdentifier &id, const DBParameters &params, Process &proc,
        const AccessCredentials *cred, const AclEntryPrototype *owner);
	virtual ~KeychainDatabase();

	KeychainDbCommon &common() const;
	const char *dbName() const;
	
public:	
	static const int maxUnlockTryCount = 3;

public:
    const DbIdentifier &identifier() const { return common().identifier(); }
	
public:
	// encoding/decoding databases
	DbBlob *blob();
	KeychainDatabase(const DLDbIdentifier &id, const DbBlob *blob, Process &proc,
        const AccessCredentials *cred);
    void authenticate(const AccessCredentials *cred);
    void changePassphrase(const AccessCredentials *cred);
	RefPointer<Key> extractMasterKey(Database &db, const AccessCredentials *cred,
		const AclEntryPrototype *owner, uint32 usage, uint32 attrs);
	void getDbIndex(CssmData &indexData);
	
	// lock/unlock processing
	void lockDb();											// unconditional lock
	void unlockDb();										// full-feature unlock
	void unlockDb(const CssmData &passphrase);				// unlock with passphrase

	bool decode();											// unlock given established master key
	bool decode(const CssmData &passphrase);				// set master key from PP, try unlock

	bool validatePassphrase(const CssmData &passphrase) const; // nonthrowing validation
	bool isLocked() const { return common().isLocked(); }	// lock status
    void notify(NotificationEvent event) { return common().notify(event); }
    void activity() const { common().activity(); }			// reset timeout clock
	
	// encoding/decoding keys
    void decodeKey(KeyBlob *blob, CssmKey &key, void * &pubAcl, void * &privAcl);
	KeyBlob *encodeKey(const CssmKey &key, const CssmData &pubAcl, const CssmData &privAcl);
	
    bool validBlob() const	{ return mBlob && version == common().version; }

	// manage database parameters
	void setParameters(const DBParameters &params);
	void getParameters(DBParameters &params);
    
    // ACL state management hooks
	void instantiateAcl();
	void changedAcl();
	const Database *relatedDatabase() const; // "self", for SecurityServerAcl's sake

    // debugging
    IFDUMP(void dumpNode());

protected:
	RefPointer<Key> makeKey(const CssmKey &newKey, uint32 moreAttributes,
		const AclEntryPrototype *owner);

	void makeUnlocked();							// interior version of unlock()
	void makeUnlocked(const AccessCredentials *cred); // like () with explicit cred
	void makeUnlocked(const CssmData &passphrase);	// interior version of unlock(CssmData)
	
	void establishOldSecrets(const AccessCredentials *creds);
	void establishNewSecrets(const AccessCredentials *creds, SecurityAgent::Reason reason);
	
	static CssmClient::Key keyFromCreds(const TypedList &sample);
	
	void encode();									// (re)generate mBlob if needed

private:
	// all following data is locked by the common lock
    bool mValidData;				// valid ACL and params (blob decoded)
        
    uint32 version;					// version stamp for blob validity
    DbBlob *mBlob;					// database blob (encoded)
    
    AccessCredentials *mCred;		// local access credentials (always valid)
};

#endif //_H_KCDATABASE
