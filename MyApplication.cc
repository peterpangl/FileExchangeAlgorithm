//
// Copyright (C) 2009 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
 * @author Antonio Zea
 * @author Panagopoulos Petros
 */

#include "UnderlayConfigurator.h"
#include "GlobalStatistics.h"
#include "GlobalNodeList.h"
#include "MyMessage_m.h"

#include "MyApplication.h"

#include <iostream>

Define_Module(MyApplication);

list<SeederNode> Seeders;      	// list of Seeders including their ip address
list<LeecherNode> Leechers;     // list of Leechers
int numNode = 0;

// initializeApp() is called when the module is being created.
// Use this function instead of the constructor for initializing variables.
void MyApplication::initializeApp(int stage)
{
    // initializeApp will be called twice, each with a different stage.
    // stage can be either MIN_STAGE_APP (this module is being created),
    // or MAX_STAGE_APP (all modules were created).
    // We only care about MIN_STAGE_APP here.

    if (stage != MIN_STAGE_APP) return;

    cout << "--->>--->>initializeApp" << endl;

	IPvXAddress ipAddr = thisNode.getIp();          // node ip
    
    // copy the module parameter values to our own variables
    sendPeriod = par("sendPeriod");     		//defined in omnetpp.ini
    numToSend = par("numToSend");
	numSeeders = par("numSeeders");
	numPktFile = par("numPktFile");
	sendInfoPeriod = par("sendInfoPeriod");
	metric = par("metric");				//defined in MyApplication.ned


    if(flag < numSeeders){
        //this node will be a seeder
    	SeederNode seeder;
        seeder.seederIp = ipAddr;      			//push this ip to seeders list
        seeder.gotInfo = 0;
        Seeders.push_back(seeder);
        cout << "this node: " << seeder.seederIp << " initialized as seeder!" << endl;
    }
    else{
        //this node will be a leecher
        LeecherNode lee;
        lee.leecherIp = ipAddr;
        lee.cntLPkt = 0;
        lee.flagLee = 0;
        lee.gotInfo = 0;
        Leechers.push_back(lee);
        cout << "this node: " << ipAddr << " initialized as leecher!" << endl;

        // start our timerMsg!
        timerMsg = new cMessage("MyApplication Timer");
        scheduleAt(simTime() + sendPeriod, timerMsg);

        // start our timerInfoMsg
        timerInfoMsg = new cMessage("MyApplication InfoTimer");
        scheduleAt(simTime() + sendInfoPeriod, timerInfoMsg);


    }
    flag++;

    // initialize our statistics variables
    numSent = 0;
    numReceived = 0;
    xorSend = 0;
    xorRcv = 0;
    broadSnd = 0;
    broadRcv = 0;
    reqSnd = 0;
    avlSnd = 0;
    avlRcv = 0;
    notAvlRcv = 0;
    numDec = 0;
    putInEnc = 0;
    hasBoth = 0;			  // has both ids from the encoded combination
    notAvlSnd = 0;
    XORfailedToSend =0;
    ignored = 0;

    RecFile = registerSignal("RecFile");

    // tell the GUI to display our variables
    WATCH(numSent);
    WATCH(numReceived);
    WATCH(xorSend);
    WATCH(xorRcv);
    WATCH(broadSnd);
    WATCH(broadRcv);
    WATCH(reqSnd);
    WATCH(avlSnd);
    WATCH(avlRcv);
    WATCH(notAvlRcv);
    WATCH(numDec);
    WATCH(putInEnc);
    WATCH(hasBoth);			  // has both ids from the encoded combination
    WATCH(notAvlSnd);
    WATCH(XORfailedToSend);
    WATCH(ignored);

    bindToPort(2000);
    
}


// finish is called when the module is being destroyed
void MyApplication::finishApp()
{
    // finish() is usually used to save the module's statistics.
    // We'll use globalStatistics->addStdDev(), which will calculate min, max, mean and deviation values.
    // The first parameter is a name for the value, you can use any name you like (use a name you can find quickly!).
    // In the end, the simulator will mix together all values, from all nodes, with the same name.
	cout << "------>finishApp" << endl;
    globalStatistics->addStdDev("MyApplication: Sent packets", numSent);
    globalStatistics->addStdDev("MyApplication: Received packets", numReceived);
    globalStatistics->addStdDev("MyApplication: Sent XORpackets", xorSend);
    globalStatistics->addStdDev("MyApplication: Received XORpackets", xorRcv);
    globalStatistics->addStdDev("MyApplication: Sent Broadpackets", broadSnd);
    globalStatistics->addStdDev("MyApplication: Received Broadpackets", broadRcv);
    globalStatistics->addStdDev("MyApplication: Sent REQpackets", reqSnd);
    globalStatistics->addStdDev("MyApplication: Sent AVLpackets", avlSnd);
    globalStatistics->addStdDev("MyApplication: Received AVLpackets", avlRcv);
    globalStatistics->addStdDev("MyApplication: Sent notAvlpackets", notAvlSnd);
    globalStatistics->addStdDev("MyApplication: Received notAvlpackets", notAvlRcv);
    globalStatistics->addStdDev("MyApplication: Decoded packets", numDec);
    globalStatistics->addStdDev("MyApplication: Put In Encoded Buf", putInEnc);
    globalStatistics->addStdDev("MyApplication: ExistBothFromCombination", hasBoth);
    globalStatistics->addStdDev("MyApplication: XORfailedToSend", XORfailedToSend);
    globalStatistics->addStdDev("MyApplication: Ignored Msg", ignored);

}


// handleTimerEvent is called when a timer event triggers
void MyApplication::handleTimerEvent(cMessage* msg)
{

    // is this our timer?
    if (msg == timerMsg) {

        // reschedule our message , only leechers send REQ
    	if(!isSeeder()){
    		scheduleAt(simTime() + sendPeriod, timerMsg);
        }

        // if the simulator is still busy creating the network,
        // let's wait a bit longer
        if (underlayConfigurator->isInInitPhase()) return;
        
        if( isSeeder() ) { // do this check for nodes who were leechers, have schedule an event and meanwhile became seeders
        	return;
        }
        else {//is leecher
            cout << endl <<endl << "----->handleTimerEvent" << endl<< endl;
            cout << "node: " << thisNode.getIp() << " is Leecher!" << endl;
            
            int i;
            //do stuff for leecher to send REQ
            for ( i = 0; i < numToSend; i++ ) {

            	// find thisNode inside leecher's list
            	list<LeecherNode>::iterator lit;
            	for( lit = Leechers.begin(); lit != Leechers.end(); lit++ ) {
            		if( lit->leecherIp == thisNode.getIp() ) {
            			break;
            		}
            	}

            	//msg
        		MyMessage *myMsg;
        		myMsg = new MyMessage();
        		myMsg->setType(REQ);				// set the msg type to REQ
        		myMsg->setSenderAddress(thisNode);  // set the sender address to our own
        		myMsg->setIdsArraySize(0);			// set ids array size to 0, if needs will change below
        		myMsg->setByteLength(100);          // set the message length to 100 bytes

            	// if leecher sends REQ first time
            	if( lit->flagLee == 0 ) { 					// send REQ to a random Seeder

            		//get a random seeder
            		int j = getRandomSeeder();

					OverlayKey randomKey(j); 				// create the random overlay key that has been found
            		lit->flagLee++;
            		numSent++;
            		reqSnd++;
            		EV << "Leecher :" << thisNode.getIp() << ": sending REQ to "
            		                        << randomKey << " Seeder!" << std::endl;
					cout << endl << " sends REQ to Seeder: 1.0.0." << j << endl;

                	callRoute(randomKey, myMsg);           	// send it to the overlay

            	}
            	else {										// send REQ to a random node, seeder or leecher

            		// get the address of one of the other nodes
					TransportAddress* addr = globalNodeList->getRandomAliveNode();
					while (thisNode.getIp().equals(addr->getIp())) {
						addr = globalNodeList->getRandomAliveNode();
					}

					IPvXAddress nodeIp = addr->getIp();
					string sendToIp = nodeIp.str().substr(6); // the node 10 if ip is 1.0.0.10
					int j = atoi(sendToIp.c_str()); 		  // string to int

					OverlayKey randomKey(j); // create the random overlay key that has been found

            		list<int>::iterator bufIt;
					unsigned int k;
            		//int bufId;
            		//cout << thisNode.getIp() << " has:" << endl;

            		// pass to the msg all ids the node has
					for(k=0, bufIt = lit->lBuf.begin(); bufIt != lit->lBuf.end(); bufIt++, k++){
					//	cout<< bufId << " ";
						myMsg->setIdsArraySize(k+1);			// set the size of ids array
						myMsg->setIds( k, *bufIt );		// set the ids
					}

            		cout << endl <<" sends REQ to node: " << nodeIp <<endl;
            		EV << "Leecher :" << thisNode.getIp() << ": sending REQ to "
            		                        << randomKey << " node!" << std::endl;
            		numSent++;
            		reqSnd++;
                	callRoute(randomKey, myMsg);                // send it to the overlay
            	}
            }
        }
    }
    /******************************** BROADCAST MESSAGE *********************************/
    else if(msg == timerInfoMsg){
    	cout <<endl<< "----------------timerInfoMsg-----------------" << endl;
    	cout << "node: " << thisNode.getIp()<< endl;
    	if( isSeeder() ) { // do this check for nodes who were leechers, have schedule an event and meanwhile became seeders
    		return;
    	}
    	if(Leechers.size() == 1){ // if i am the only one leecher don't broadcast msg
    		cout << "i'm the only one leecher in neighborhood no need to broadcast" << endl;
    		delete msg;
    	}
    	else{
    		scheduleAt(simTime() + sendInfoPeriod, timerInfoMsg); // reschedule the timerInfoMsg
    		int flagFound = 0;
    		// find thisNode inside leecher's list
    		list<LeecherNode>::iterator lit;
    		for( lit = Leechers.begin(); lit != Leechers.end(); lit++ ) {
    			if( lit->leecherIp == thisNode.getIp() ) {
    				flagFound = 1;
    				break;
    			}
    		}
    		if(flagFound && lit->lBuf.size() != 0){		// broadcast the msg
    			broadcastInfoMsg(lit); 					// at Functions.cc
    		}
    		else{
    			cout << " not broadcast yet!" << endl << "my Buffer Size: " << lit->lBuf.size() << endl;
    		}
    	}
    }
    else {
        cout << endl<<" unknown message types are discarded " << endl;
        delete msg;
    }
}





// deliver() is called when we receive a message from the overlay.
// Unknown packets can be safely deleted here.
void MyApplication::deliver(OverlayKey& key, cMessage* msg)
{    
    
    cout << endl<< endl << "----->deliver" << endl;
    
    // we are only expecting messages of type MyMessage, throw away any other
    MyMessage *myMsg = dynamic_cast<MyMessage*>(msg);
    if ( myMsg == NULL ) {
    	cout << "NULL MSG" << endl;
        delete msg; // type unknown!
        return;
    }

    /************************** REQ ***************************/
    if ( myMsg->getType() == REQ ){
    	numReceived++;
    	int pktId;

    	/********* SEEDER HERE ************/

    	if( isSeeder() ){
    		cout << endl << "Seeder: " << thisNode.getIp() << " Got Req from " << myMsg->getSenderAddress().getIp() << endl;

    		// print all ids that thisNode has
    		cout<< thisNode.getIp() << " has these ids:"<< endl;
    		for(int g=0; g<numPktFile; g++){
    			cout << g+1 << " ";
			}
			cout << endl;

			// Requesting node has no packet yet
    		if( myMsg->getIdsArraySize() == 0 ){
    			cout << myMsg->getSenderAddress().getIp() <<" hasn't ids yet " << endl;
    			//choose a random packetId
    			pktId = rand() % numPktFile + 1;
    		}
    		// Requesting node has already some packets
    		else {
    			vector<int> tmp1Ids; // keep ids of the received msg
    			unsigned int i;

    			cout << myMsg->getSenderAddress().getIp()<< " has these ids:" << endl;
    			// put the ids of msg in tmp1Ids
    			for( i = 0; i < myMsg->getIdsArraySize(); i++ ) {
    				tmp1Ids.push_back( myMsg->getIds(i) );
    				cout << tmp1Ids.at(i) << " ";
    			}
    			cout << endl;

    			pktId  = seederFindDiffId(tmp1Ids); // pktId is the id to send

    		}

    		TransportAddress sendAVLTo = myMsg->getSenderAddress();
    		//fix message
	        myMsg->setType(AVL);
	        myMsg->setIdsArraySize(1);
	        myMsg->setIds(0, pktId);
	        myMsg->setSenderAddress(thisNode);
	        myMsg->setByteLength(16384);

	        EV <<endl<< "send AVL to "<< sendAVLTo.getIp() << " , pktId: " << pktId << endl;
    		cout <<endl<< "send AVL to "<< sendAVLTo.getIp() << " , pktId: " << pktId << endl;

	        numSent++;
	        avlSnd++;
	        sendMessageToUDP(sendAVLTo, myMsg);

    	}

    	/********* LEECHER HERE ************/

    	else {//A leecher received a REQ

    		cout << endl <<"Leecher " << thisNode.getIp() << " Got Req from " << myMsg->getSenderAddress().getIp() << endl;
			list<LeecherNode>::iterator lit;
			list<int>::iterator bufIt;
			// find thisNode inside leecher's list
			for (lit = Leechers.begin(); lit != Leechers.end(); lit++) {
				if (lit->leecherIp == thisNode.getIp()) {
					break;
				}
			}

			// print all ids that thisNode has
			cout << thisNode.getIp() << " has these ids:" << endl;
			for (bufIt = lit->lBuf.begin(); bufIt != lit->lBuf.end(); bufIt++) {
				cout << *bufIt << " ";
			}
			cout << endl;

			TransportAddress sendBackTo = myMsg->getSenderAddress();
			if (lit->lBuf.size() == 0) { // thisNode hasn't any packet id in buffer

				myMsg->setType(NOT_AVL);
				myMsg->setIdsArraySize(0);
				myMsg->setSenderAddress(thisNode);
				myMsg->setByteLength(100);
				notAvlSnd++;
				cout << endl << "send NOT_AVL to " << sendBackTo.getIp() << endl;
			}
			else { // thisNode has some packets in buffer
				vector<int> tmp1Ids;
				vector<int> tmp2Ids;	// missing ids, possible to send
				unsigned int i;

    			cout << myMsg->getSenderAddress().getIp()<< " has these ids:" << endl;

				for (i = 0; i < myMsg->getIdsArraySize(); i++) {						// copy received ids in tmp1Ids
					tmp1Ids.push_back(myMsg->getIds(i));
					cout << tmp1Ids.at(i) << " ";
				}
				cout << endl;

				if (tmp1Ids.size() != 0) { 												// requesting node has already some packets
					for (bufIt = lit->lBuf.begin(); bufIt != lit->lBuf.end(); bufIt++) {// find my not equal ids with thisNode
						for (i = 0; i < tmp1Ids.size(); i++) {
							if ( tmp1Ids.at(i) == *bufIt ) { 							// are equal
								break;
							}
							if( tmp1Ids.at(i) > *bufIt ){								// two nodes have different id
								//cout << "possible to send --------------------->" << *bufIt << endl;
								tmp2Ids.push_back(*bufIt);								// possible id to send
								break;
							}
						}
						if (i == tmp1Ids.size()) {// two nodes have different id
							tmp2Ids.push_back(*bufIt); // put *bufIt in tmp2Ids
							//cout << "possible id to send : " << *bufIt << endl;
						}
					}
					if (tmp2Ids.size() != 0) {											// there were different ids between two nodes
						// get a random id
						int sz = tmp2Ids.size();
						int r = rand() % sz + 1;
						pktId = tmp2Ids.at(r - 1);

						//fix message
						myMsg->setType(AVL);
						myMsg->setIdsArraySize(1);
						myMsg->setIds(0, pktId);
						myMsg->setByteLength(16384);
						avlSnd++;
						EV << endl << "send AVL to " << sendBackTo.getIp()
														<< " , pktId: " << pktId << endl;
						cout << endl << "send AVL to " << sendBackTo.getIp()
								<< " , pktId: " << pktId << endl;
					}
					else { // all ids were equal
						//fix message
						myMsg->setType(NOT_AVL);
						myMsg->setIdsArraySize(0);
						myMsg->setByteLength(100);
						notAvlSnd++;
						cout << endl << "send NOT_AVL to " << sendBackTo.getIp() << endl;
					}
					myMsg->setSenderAddress(thisNode);
				}
				else { // Requesting node has no packet yet
					//choose a random id and send
					int sz = lit->lBuf.size();
					int r = rand() % sz +1;
					for(sz=0, bufIt = lit->lBuf.begin(); sz < r-1; bufIt++,sz++){
					}
					pktId = *bufIt;
					//fix message
					myMsg->setType(AVL);
					myMsg->setIdsArraySize(1);
					myMsg->setIds(0, pktId);
					myMsg->setSenderAddress(thisNode);
					myMsg->setByteLength(16384);
					avlSnd++;
					EV << endl <<"send AVL to "<< sendBackTo.getIp() << " , pktId: " << pktId << endl;
					cout << endl <<"send AVL to "<< sendBackTo.getIp() << " , pktId: " << pktId << endl;
				}
			}
			numSent++;
			//send msg
			sendMessageToUDP(sendBackTo, myMsg);
    	}
    }// end REQ
    else {
    	cout << thisNode.getIp() <<" unknown Type " << endl;
        delete msg;
    }
}




// handleUDPMessage() is called when we receive a message from UDP.
// Unknown packets can be safely deleted here.
void MyApplication::handleUDPMessage(cMessage* msg)
{
cout <<endl << endl << "-------->handleUDPMessage" << endl << endl;
    // we are only expecting messages of type MyMessage

	MyMessage *myMsg = dynamic_cast<MyMessage*>(msg);

	if ( myMsg->getType() == NOT_AVL) {
        EV << thisNode.getIp() << ": Got NOT_AVL from: " << myMsg->getSenderAddress().getIp()<< std::endl;
        cout << thisNode.getIp() << ": Got NOT_AVL from: " << myMsg->getSenderAddress().getIp()<< std::endl;
        numReceived++;
        notAvlRcv++;

    }
	else if ( myMsg->getType() == AVL) {
		EV << thisNode.getIp() << ": Got AVL from: " << myMsg->getSenderAddress().getIp();
		cout << thisNode.getIp() << ": Got AVL from: " << myMsg->getSenderAddress().getIp();
		numReceived++;
		avlRcv++;
	    //find thisNode inside leecher's list
	    list<LeecherNode>::iterator lit;
	    for ( lit = Leechers.begin(); lit != Leechers.end(); lit++) {
	        if (lit->leecherIp == thisNode.getIp()) {
	            break;
	        }
	    }

	    if( !isSeeder() ){//thisNode is leecher until now
			int pktIdRcv = myMsg->getIds(0);

			cout << " , packet id:" << pktIdRcv << endl;

			if( !idExists(pktIdRcv, lit) ){

				// new pkt
				lit->cntLPkt++;
				lit->lBuf.push_back(pktIdRcv);
				lit->lBuf.sort();

				cout << endl <<" packet: " << pktIdRcv << " inserted in buffer of :"
						<< thisNode.getIp() << " -  "
						<< ( (float) lit->cntLPkt / (float) numPktFile ) * 100 <<"% complete." << endl;

				afterInsertingInLBuf(lit);

			}
			else{
				// this packet exists already
				cout << " this packet:" << pktIdRcv << " exists already in leecher:"
						<< thisNode.getIp() << endl;
			}
		}
		else{// node became seeder
			//ignore message
			//EV << thisNode.getIp() << " has the whole file, so ignore message" << endl;
			ignored++;
			cout << endl << thisNode.getIp() << " is seeder, has the whole file, so ignore the message" << endl;
		}
	}
	else if(myMsg->getType() == XOR){
    	numReceived++;
    	xorRcv++;

    	if(isSeeder()){
    		// ignore it
    		ignored++;
    		cout << thisNode.getIp() << " got a XOR msg but became a SEEDER in the meanwhile of process "<< endl;
    	}
    	else{
    		// handle with xor msg
    		list<LeecherNode>::iterator lit;
    		// find & check if thisNode.ip exists in leechers list
    		for( lit = Leechers.begin(); lit != Leechers.end(); lit++) {
    			if( lit->leecherIp == thisNode.getIp() ) {
    				break;
    		    }
    		}
    		// handle xor msg
    		cout << lit->leecherIp << " got XOR msg from "<< myMsg->getSenderAddress().getIp() << endl;
    		handleXORmsg(myMsg, lit);			// at Functions.cc
    	}
	}
	/********************************RECEIVE BROADCAST MESSAGE *********************************/

	else if( myMsg->getType() == INFO ){				// network coding part
		numReceived++;
		broadRcv++;

		if( isSeeder() ){
			/***************** SEEDER HERE ************/

			cout << endl <<"Seeder " << thisNode.getIp() << " Got INFO from " << myMsg->getSenderAddress().getIp() << endl;
			if(Leechers.size() == 1){
				// if only one leecher exist, no necoding
				cout << " only one leecher in neighborhood, no need for coding " << endl;
				delete msg;
				return;
			}
			list<SeederNode>::iterator sit;
			// find & check if thisNode.ip exists in seeders list
			for( sit = Seeders.begin(); sit != Seeders.end(); sit++) {
				if( sit->seederIp == thisNode.getIp() ) {
					break;
				}
			}

			sit->gotInfo++;									// counts the INFO msgs receives this node
			cout << thisNode.getIp() << " go to construct your Bases" << endl;

			constructBasesOfSeeder(myMsg, sit);				// Common and notHas bases at SeederInfoMsgMngmt.cc

			if( sit->gotInfo == Leechers.size() ){			// if has received msg from all leechers
				cout << endl << thisNode.getIp() << " received INFO from all leechers !!!!!!!!!!!!!!!!!!!!" << endl;
				simtime_t startFPNI = simTime();
				cout << "startFPNI: "<< startFPNI<< endl;
				if (findPossibleNewId(sit) == 0){		// notHas base is empty
					clear(sit);
					cout <<" there is no new id for nodes to xor" << endl;
					return;
				}
				simtime_t stopFPNI = simTime();
				cout << "stopFPNI: "<< stopFPNI<< endl;

				// after returning from findPossibleNewId, we have found the ids that are missing from nodes and their score which
				// inform us about from how many nodes is missing. These ids with their score exist in notHasIds_Score base
				// now sort this base in descending order.first the id with greatest score etc..

				sortNotHasIds_ScoreBase(sit);
				//printNotHasIds_Score(sit);

				simtime_t startCIXC = simTime();
				cout << "startCIXC: " << startCIXC << endl;
				if(commonInXorCombination(sit)){
					cout<< "OK XOR multicast message has been sent "<< endl;
				}
				else{
					cout <<" XOR multicast message hasn't been sent"<< endl;
					XORfailedToSend++;
				}
				simtime_t stopCIXC = simTime();
				cout << "stopCIXC: " << stopCIXC << endl;

				// after sending or not the xor msg
				//clear list for next round of broadcasts messages
				clear(sit);
			}
		}
		else {

			/****************** LEECHER HERE *****************/

			cout << endl <<"Leecher " << thisNode.getIp() << " Got INFO from " << myMsg->getSenderAddress().getIp() << endl;

			if(Leechers.size() == 2){ 				// if exist only two leechers -me and someone else-, no reason for network coding
				cout << "only one leecher(except me) exist in neighborhood, no reason for network coding " << endl;
				delete msg;
				return;
			}
			list<LeecherNode>::iterator lit;
			// find & check if thisNode.ip exists in leechers list
			for( lit = Leechers.begin(); lit != Leechers.end(); lit++) {
				if( lit->leecherIp == thisNode.getIp() ) {
					break;
				}
			}
			lit->gotInfo++;									// counts the INFO msgs receives this node
			cout << thisNode.getIp() << " go to construct your Bases" << endl;

			constructBasesOfLeecher(myMsg, lit);			// Common and notHas bases at LeecherInfoMsgMngmt.cc


			if( lit->gotInfo == Leechers.size()-1 ){

				cout << endl << thisNode.getIp() << " received INFO from rest leechers !!!!!!!!!!!!!!!!!!!!" << endl;
				if (findPossibleNewId(lit) == 0){		//notHas base is empty
					clear(lit);
					cout <<" there is no new id for nodes to xor" << endl;
					return;
				}
				// after returning from findPossibleNewId, we have the ids that are missing from nodes and their score which
				// inform us about from how many nodes is missing

				sortNotHasIds_ScoreBase(lit);
				//printNotHasIds_Score(lit);

				if(commonInXorCombination(lit)){
					cout<< "OK XOR multicast message has been sent "<< endl;
				}
				else{
					cout <<" XOR multicast msg hasn't been sent"<< endl;
					XORfailedToSend++;
				}
				// after sending the xor msg
				//clear list for next round of broadcasts messages
				clear(lit);
			}
		}
	}
	else{
		cout << "Unknown Type" << endl;
	}
    // Message not needed anymore -> delete it
    delete msg;
}
