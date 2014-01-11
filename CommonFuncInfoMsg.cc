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


// check if we have already compute the score of this id
int MyApplication::checkIfExist(int s, list<struct idScore> X){
	//cout<<"-------checkIfExist------" << endl;
    list<struct idScore>::iterator it;
    int so;

    for(it = X.begin(); it != X.end(); it++){
        so = it->pktId;
        //cout << "it->pktId " << so << " compared with pktid param " << s <<  endl;
        if(so == s){
            //cout << s << " already exists" << endl;
            return 1;
        }
    }
    return 0;

}

// xor the packets new and common and send msg
void MyApplication::xorPktsANDSend(int newPkt, int commonPkt, list<IPvXAddress> nodes){
	cout << endl <<endl<<"-!!-!-!-!-!-!!-xorPktsANDSend-----------"<< endl;
	list<IPvXAddress>::iterator ipIt;
	
	cout <<" newPkt: "<< newPkt<< " & commonPkt: "<< commonPkt << endl;

	// msg
	MyMessage *myMsg;
	myMsg = new MyMessage();
	myMsg->setType(XOR);		  		// set the msg type to XORED_IDS
	myMsg->setSenderAddress(thisNode);  	// set the sender address to our own
	myMsg->setByteLength(16384);	            // set the message length to 100 bytes

	// pass to the msg the ids's combination
	myMsg->setXoredIdsArraySize(2);		// set the size of ids array
	myMsg->setXoredIds( 0, commonPkt );		// set the commonPkt id
	myMsg->setXoredIds( 1, newPkt );			// set the newPkt id

	//int toNode;
	getParentModule()->getParentModule()->bubble("XOR MSG");
	EV << thisNode.getIp() << " sendMessageToUDP pkt:"<< newPkt << " ^ "<< commonPkt << " to ";
	cout << thisNode.getIp() << " sendMessageToUDP pkt:"<< newPkt << " ^ "<< commonPkt << " to ";
	for(ipIt = nodes.begin(); ipIt != nodes.end(); ipIt++){

		EV << endl << *ipIt;
		cout << endl <<*ipIt;

		MyMessage *copyMsg = myMsg->dup();

		const TransportAddress remoteAddress = TransportAddress(*ipIt, 2000);
		numSent++;
		xorSend++;
		sendMessageToUDP(remoteAddress, copyMsg);

	}
	cout <<endl;
}
