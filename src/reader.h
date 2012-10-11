/*
 * Copyright (c) 2004 Apple Computer, Inc. All Rights Reserved.
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
// reader - token reader objects
//
#ifndef _H_READER
#define _H_READER

#include "securityserver.h"
#include "structure.h"
#include "token.h"
#include "pcsc++.h"


//
// A Reader object represents a token (card) reader device attached to the
// system.
//
class Reader : public PerGlobal {
public:
	Reader(const PCSC::ReaderState &state);
	~Reader();
	
	void kill();
	
	string name() const { return mName; }
	const PCSC::ReaderState &pcscState() const { return mState; }
	
	void update(const PCSC::ReaderState &state);
	
	IFDUMP(void dumpNode());
	
protected:
	void transit(const PCSC::ReaderState &state);
	void insertToken();
	void removeToken();
	
private:
	string mName;		// PCSC reader name
	PCSC::ReaderState mState; // name field not valid (use mName)
	Token *mToken;		// token inserted here (also in references)
};


#endif //_H_READER
