/**
 * @author Panagopoulos Petros
 */

#include "UnderlayConfigurator.h"
#include "GlobalStatistics.h"
#include "GlobalNodeList.h"
#include "MyMessage_m.h"

#include "MyApplication.h"

#include <iostream>

Define_Module(MyApplication);

//returns true if this node is seeder
bool MyApplication::isSeeder() {

    list<SeederNode>::iterator it;
    // check if thisNode.ip exists in seeders list
    for( it = Seeders.begin(); it != Seeders.end(); it++) {
        if( it->seederIp == thisNode.getIp() ) {
        	//cout << "isSeeder EXIST this SEEDER" << endl;
        	return true;
        }
    }
    return false;
}


// checks if idExists already in leecher's buffer
bool MyApplication::idExists(int pktId, list<LeecherNode>::iterator it){

    //cout << "----->idExists()? pktRcv: " << pktId << endl;

    list<int>::iterator bufIt;
    for (bufIt = it->lBuf.begin(); bufIt != it->lBuf.end(); bufIt++) {
        //cout << "Already Exists() : " << *bufIt << endl;
        if( pktId == *bufIt ) {
            //the ids are equal, so we have the packet
            return true;
        }
		if( *bufIt > pktId ){
			//we haven't the packet
			break;
		}
    }
    return false;
}


// find a random different from the other's node id
int MyApplication::seederFindDiffId(vector<int> tmp1Ids){

	vector<int> tmp2Ids; // missing ids, possible to send
	int k;
	unsigned int j;
	int thisId;

	for( thisId = 1; thisId <= numPktFile; thisId++ ) {
		for( j = 0; j < tmp1Ids.size(); j++ ){
			if( thisId == tmp1Ids.at(j) ){//are equal
				break;
			}
		}
		if( j == tmp1Ids.size() ) {		// two nodes have separate id
//			cout << "::::::::::::::possible id to send: " << thisId << endl;
			tmp2Ids.push_back(thisId); 	// put thisId in tmp2
		}
	}
	// tmp2Ids never empty because thisNode is seeder and the other is leecher
	// get a random id
	int sz = tmp2Ids.size();
	k = rand() %sz +1;
	return tmp2Ids.at(k-1);
}


//find a random seeder
int MyApplication::getRandomSeeder() {

	list<SeederNode>::iterator sit;
	int sz = Seeders.size();
	if (sz == 0) {cout << "noSeeDeR?? :S" << endl;	endSimulation();	}
	int r = rand() % sz + 1; //r>=1
	int i;
	for (i = 0, sit = Seeders.begin(); i < r-1; i++, sit++) {
	}
	//sit->seederIp has ip address of random seeder
	IPvXAddress ip = sit->seederIp; // ip address of seeder
	string sendToIp = ip.str().substr(6); // the node 10 if ip is 1.0.0.10
	int j = atoi(sendToIp.c_str()); // string to int

	return j;
}



// send to all neighbors of neighborhood what i have in my buffer
void MyApplication::broadcastInfoMsg(list<LeecherNode>::iterator lit) {
	cout << "--------------------broadcastInfoMsg----------------------" << endl;
	int k;
	list<int>::iterator bufIt;

	// msg
	MyMessage *myMsg;
	myMsg = new MyMessage();
	myMsg->setType(INFO);				// set the msg type to INFO
	myMsg->setSenderAddress(thisNode);  // set the sender address to our own

	// pass to the msg all ids that i have
	for(k=0, bufIt = lit->lBuf.begin(); bufIt != lit->lBuf.end(); bufIt++, k++){
		myMsg->setIdsArraySize(k+1);	// set the size of ids array
		myMsg->setIds( k, *bufIt );		// set the ids
	}
	myMsg->setByteLength(100);          // set the message length to 100 bytes

	IPvXAddress nodeIp = thisNode.getIp();

	EV << "Leecher :" << thisNode.getIp() << ": broadcasts to its neighbors" << endl;
	cout << "Leecher :" << thisNode.getIp() << ": broadcasts to its neighbors:" << endl;
	getParentModule()->getParentModule()->bubble("SEND BROADCAST MSG");

	list<LeecherNode>::iterator leeIt;
	list<SeederNode>::iterator seeIt;

	for(leeIt = Leechers.begin(); leeIt!=Leechers.end(); leeIt++){
		if(leeIt->leecherIp != lit->leecherIp){
			MyMessage *copyMsg = myMsg->dup();
			const TransportAddress remoteAddress = TransportAddress(leeIt->leecherIp, 2000);
			cout <<" " << leeIt->leecherIp;
			EV <<" " << leeIt->leecherIp;
			broadSnd++;
			numSent++;
			sendMessageToUDP(remoteAddress, copyMsg);
		}
	}
	for(seeIt = Seeders.begin(); seeIt!=Seeders.end(); seeIt++){
		MyMessage *copyMsg = myMsg->dup();
		const TransportAddress remoteAddress = TransportAddress(seeIt->seederIp, 2000);
		cout <<" "<< seeIt->seederIp;
		EV <<" "<< seeIt->seederIp;
		broadSnd++;
		numSent++;
		sendMessageToUDP(remoteAddress, copyMsg);
	}
}


// check if the xoredIds are there in encBuf already
int MyApplication::checkInEncBuf(int a, int b, list<LeecherNode>::iterator lit){
	list<struct xored>::iterator it;

	for(it = lit->encBuf.begin(); it != lit->encBuf.end(); it++){
		if( (it->ids[0] == a && it->ids[1] == b) || (it->ids[0] == b && it->ids[1] == a) ){
			cout << a<<","<<b<< " exist in EncBuf"<< endl;
			return 1; // exist in encBuf
		}
	}
	return 0;	// don't exist in encBuf
}

//print the ids of node
void nodeHasIds(list<LeecherNode>::iterator lit){
	list<int>::iterator bufIt;
	cout<< endl<< lit->leecherIp<< " has:"<< endl;
	for (bufIt = lit->lBuf.begin(); bufIt != lit->lBuf.end(); bufIt++) {
		cout << *bufIt << " ";
	}
}

// checks if the node has received the whole file
int MyApplication::receivedFile(list<LeecherNode>::iterator lit){
	// check if all the file received
	cout << "received whole file ? " << endl<<endl;
	if(lit->cntLPkt == numPktFile){
		nodeHasIds(lit);
		numNode++;
        emit(RecFile, numNode);		// Complete File Reception for stats

		cout << endl <<"!!!!!!the file received, " << thisNode.getIp() << " going to be a seeder!!!!!!" << endl;

		simtime_t simT = simTime();
		double endTime = simT.dbl();
		globalStatistics->addStdDev("MyApplication: Complete Reception", endTime);

		// make thisNode a seeder
		SeederNode seeder;
		seeder.seederIp = thisNode.getIp();
		Seeders.push_back(seeder);
		Leechers.erase(lit);

		if(Leechers.size() == 0){
			//end simulation as all nodes have the packet.
			EV << "the file has been received successfully from all users. " << endl;
			cout << endl << "the file has been received successfully from all users. " << endl;
			cout << endl << "End simulation" << endl;

			endSimulation();
		}
		else{
			//print the rest leechers
			cout << "rest leechers: " << endl;

			for(lit = Leechers.begin(); lit != Leechers.end(); lit++){
				cout << lit->leecherIp << endl;
			}
		}
		return 1;
	}
	cout<< "the file hasn't been received yet"<< endl;
	return 0;
}

// must check if this leecher has any id in lBuf that is the same with any id of the xored pair combination in encBuf
// if yes decode the combination, push back in lBuf, increase the counter, sort the list and return 1;
// if not return 0;
int MyApplication::canDecodeFromEncBuf(list<LeecherNode>::iterator lit){
	list<struct xored>::iterator it;
	cout << "check for decoding "<< endl;
	if(lit->encBuf.size() == 0){
		cout << "encoded buffer of "<< lit->leecherIp << " is empty " << endl;
		return 0;
	}
	else{
		for(it = lit->encBuf.begin(); it != lit->encBuf.end(); it++){
			if( (binary_search(lit->lBuf.begin(), lit->lBuf.end(), it->ids[0])) && (!binary_search(lit->lBuf.begin(), lit->lBuf.end(), it->ids[1])) ){
				cout << it->ids[0] << " exist and " << it->ids[1] << " doesn't exist in lBuf" << endl << "decode " << it->ids[1]<< endl;

				lit->cntLPkt++;
				lit->lBuf.push_back(it->ids[1]);									// put it in buffer lit->lBuf
				lit->lBuf.sort();													// sort elements
				numDec++;
				cout << endl <<" packet: " << it->ids[1] << " decoded and inserted in buffer of :"
					 << lit->leecherIp << " -  "
					 << ( (float) lit->cntLPkt / (float) numPktFile ) * 100 <<"% complete." << endl;

				lit->encBuf.erase(it);												// erase this combination of encoded ids from EncBuf
				return 1;
			}
			if( (!binary_search(lit->lBuf.begin(), lit->lBuf.end(), it->ids[0])) && (binary_search(lit->lBuf.begin(), lit->lBuf.end(), it->ids[1])) ){
				cout << it->ids[1] << " exist and " << it->ids[0] << " doesn't exist in lBuf" << endl << "decode " << it->ids[0]<< endl;

				lit->cntLPkt++;
				lit->lBuf.push_back(it->ids[0]);									// put it in buffer lit->lBuf
				lit->lBuf.sort();													// sort elements
				numDec++;
				cout << endl <<" packet: " << it->ids[0] << " decoded and inserted in buffer of :"
					 << lit->leecherIp << " -  "
					 << ( (float) lit->cntLPkt / (float) numPktFile ) * 100 <<"% complete." << endl;

				lit->encBuf.erase(it);												// erase this combination of encoded ids from EncBuf
				return 1;
			}
		}
		cout << "nothing to decode for " << lit->leecherIp << endl;
		return 0;
	}
}



// do all necessary things after inserting in buffer an id.These are:
// 1-check if has received the whole file, if yes be a seeder
// otherwise check if you can decode any combination from encoded buffer ( encBuf ), if yes decode and put in buffer
// and from 1 again
void MyApplication::afterInsertingInLBuf(list<LeecherNode>::iterator lit){

	if( !receivedFile(lit) ){				// node hasn't received the whole file
		if(canDecodeFromEncBuf(lit)){		// check if you can decode from encBuf
			afterInsertingInLBuf(lit);		// check if you have the whole file etc.
		}
	}
}



// handle XOR msg
void MyApplication::handleXORmsg(MyMessage *myMsg, list<LeecherNode>::iterator lit){
	int xoredIds[2];
	xoredIds[0] = myMsg->getXoredIds(0);
	xoredIds[1] = myMsg->getXoredIds(1);
	cout << " pkt: " << xoredIds[0] << " ^ " << xoredIds[1] << endl;
	//cout <<"for " << lit->leecherIp << " ";

	if( binary_search(lit->lBuf.begin(), lit->lBuf.end(), xoredIds[0]) ) { 		// the xoredIds[0] exist in decoded buffer
		cout << "pkt: "<< xoredIds[0] << " exist" << endl;
		if (!binary_search(lit->lBuf.begin(), lit->lBuf.end(), xoredIds[1])	) {	// and pkt: xoredIds[1] doesn't exist, you can decode xoredIds[1]
			cout << "pkt: "<< xoredIds[1] << " doesn't exist" << endl;
			cout << "decode pkt:" << xoredIds[1] << " and put it in Buffer"<< endl;

			lit->cntLPkt++;
			lit->lBuf.push_back(xoredIds[1]);									// put it in buffer lit->lBuf
			lit->lBuf.sort();													// sort elements

			cout << endl <<" packet: " << xoredIds[1] << " inserted in buffer of :"
				 << lit->leecherIp << " -  "
				 << ( (float) lit->cntLPkt / (float) numPktFile ) * 100 <<"% complete." << endl;

			numDec++;															// #of decoded packets

			afterInsertingInLBuf(lit);

		}
		else{		// pkt: xoredIds[1] exist
			cout << "pkt: "<< xoredIds[1] << " exist BOTH" << endl;
			hasBoth++;
			return;
		}
	}
	else{ 			// pkt: xoredIds[0] doesn't exist
		cout << "pkt: "<< xoredIds[0] << " doesn't exist" << endl;
		if ( binary_search(lit->lBuf.begin(), lit->lBuf.end(), xoredIds[1])	) {	// pkt: xoredIds[1] exist in decoded buffer, you can decode xoredIds[0]
			cout << "pkt: "<< xoredIds[1] << " exist" << endl;
			cout << "decode pkt:" << xoredIds[0]<< " and put it in Buffer "<< endl;

			lit->cntLPkt++;
			lit->lBuf.push_back(xoredIds[0]);									// put it in buffer lit->lBuf
			lit->lBuf.sort();													// sort elements

			cout << endl <<" packet: " << xoredIds[0] << " inserted in buffer of :"
				 << lit->leecherIp << " -  "
				 << ( (float) lit->cntLPkt / (float) numPktFile ) * 100 <<"% complete." << endl;

			numDec++;

			afterInsertingInLBuf(lit);

		}
		else{		// pkt: xoredIds[1] doesn't exist, xoredIds[0] and xoredIds[1] cannot be decoded
			cout <<"pkt: " << xoredIds[1] << " doesn't exist ..you cannot decode the xored "<< xoredIds[0] << "^" << xoredIds[1] << endl<< " put them in encoded Buffer"<< endl;
			struct xored ct;
			// put in encBuf only if this combination of xored ids don't exist
			if(lit->encBuf.size() != 0){
				if( !checkInEncBuf(xoredIds[0], xoredIds[1], lit) ){
					ct.ids[0] = xoredIds[0];
					ct.ids[1] = xoredIds[1];
					lit->encBuf.push_back(ct);									// put them in encBuf
					cout << " OK now are in encBuf" << endl;
				}
			}
			else{	// encBuf empty so put them in buffer
				ct.ids[0] = xoredIds[0];
				ct.ids[1] = xoredIds[1];
				lit->encBuf.push_back(ct);										// put them in encBuf
				putInEnc++;
				cout << " OK now are in encBuf" << endl;
			}
		}
	}
}
