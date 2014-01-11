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


// print Common and notHas Base of thisLeecher
void MyApplication::printBases(list<LeecherNode>::iterator lit){
    cout<< endl << " ------------------PRINT BASES OF LEECHER:" << lit->leecherIp<<"====================" << endl;

    list<struct DBase>::iterator it;	// iterator for every neighbor in Base
    list<int>::iterator pit;			// iterator for pktIds of Base
    cout << "lit->leecherIp: " << lit->leecherIp << endl;

    for (it = lit->Common.begin(); it != lit->Common.end(); it++) {
        cout << "node: " << it->nodeAddr << " hasCommon " << endl;
        for (pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++) {
            cout << *pit << " ";
        }
        cout << endl;
    }
    for (it = lit->notHas.begin(); it != lit->notHas.end(); it++) {
        cout << "node: " << it->nodeAddr << " notHas " << endl;
        for (pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++) {
            cout << *pit << " ";
        }
        cout << endl;
    }

}

// find common ids and ids that neighbor hasn't
void MyApplication::constructBasesOfLeecher(MyMessage *myMsg, list<LeecherNode>::iterator lit) {
	cout << endl;

	list<int>::iterator myId;
	vector<int> neighborIds;
	vector<int> commonIds;
	vector<int> notHasIds;
	unsigned int i;
	struct DBase ctCom;
	struct DBase ctNh;

	IPvXAddress neighborAddr = myMsg->getSenderAddress().getIp();

	//cout << neighborAddr << " has: ";
	ctCom.nodeAddr = neighborAddr;
	ctNh.nodeAddr = neighborAddr;

	for (i = 0; i < myMsg->getIdsArraySize(); i++) {		// always idsArraySize!=0 otherwise neighbor hadn't send
		neighborIds.push_back(myMsg->getIds(i));			// copy to neighbor ids
	//	cout << neighborIds.at(i) << " ";
	}
	//cout << endl;
	//cout << lit->leecherIp << " has: ";
	//for(myId = lit->lBuf.begin(); myId!=lit->lBuf.end(); myId++){ cout << *myId << " " ; }cout << endl; // for debug

	for(myId = lit->lBuf.begin(); myId!=lit->lBuf.end(); myId++){
		for (i = 0; i < myMsg->getIdsArraySize(); i++) {
			if( *myId == neighborIds.at(i) ){			// common id
				ctCom.pktIds.push_back(*myId);
				break;
			}
			if( *myId < neighborIds.at(i) ){				// neighbor hasn't myId
				ctNh.pktIds.push_back(*myId);
				break;
			}
		}
		if (i == neighborIds.size()) { 					// neighbor hasn't myId
			ctNh.pktIds.push_back(*myId); // put myId in neighborIds
		}
	}
	//put in Bases
	//Common Base
	if(ctCom.pktIds.size()!=0){
		lit->Common.push_back(ctCom);
	}
	else{
		cout << " has no common id with " << neighborAddr << endl;
	}
	//NotHas Base
	if(ctNh.pktIds.size()!=0){
		lit->notHas.push_back(ctNh);
	}
	else{
		cout << "all ids of "<< lit->leecherIp << " exist at " << neighborAddr << endl;
	}

	printBases(lit);

}


// sort in descending order regarding score
void MyApplication::sortNotHasIds_ScoreBase(list<LeecherNode>::iterator lit){

	list<struct idScore>::iterator a;
	list<struct idScore>::iterator b;
	list<struct idScore>::iterator c;
	int i,j;

	// SelectionSort
	a = lit->notHasIds_Score.end();
	for(a--, i = lit->notHasIds_Score.size()-1; i>0; a--,i--){
		c = lit->notHasIds_Score.begin();
		b = lit->notHasIds_Score.begin();
		for(b++, j=1; j<=i; b++,j++){
			if( b->score < c->score ){
				c = b;
			}
		}
		struct idScore tmp;
		//swap
		tmp.nodes = c->nodes;
		tmp.pktId = c->pktId;
		tmp.score = c->score;
		c->nodes = a->nodes;
		c->pktId = a->pktId;
		c->score = a->score;
		a->nodes = tmp.nodes;
		a->pktId = tmp.pktId;
		a->score = tmp.score;

	}
}

// sort in descending order regarding score
void MyApplication::sortCommonIds_ScoreBase(list<LeecherNode>::iterator lit){

	list<struct idScore>::iterator a;
	list<struct idScore>::iterator b;
	list<struct idScore>::iterator c;
	int i,j;

	// SelectionSort
	a = lit->commonIds_Score.end();
	for(a--, i = lit->commonIds_Score.size()-1; i>0; a--,i--){
		c = lit->commonIds_Score.begin();
		b = lit->commonIds_Score.begin();
		for(b++, j=1; j<=i; b++,j++){
			if( b->score < c->score ){
				c = b;
			}
		}
		struct idScore tmp;
		//swap
		tmp.nodes = c->nodes;
		tmp.pktId = c->pktId;
		tmp.score = c->score;
		c->nodes = a->nodes;
		c->pktId = a->pktId;
		c->score = a->score;
		a->nodes = tmp.nodes;
		a->pktId = tmp.pktId;
		a->score = tmp.score;
	}
}


void MyApplication::printNotHasIds_Score(list<LeecherNode>::iterator lit){
    cout << endl << "----------------print NOTHAS IDS score======================" << endl;

    list<struct idScore>::iterator it;
    list<IPvXAddress>::iterator ipIt;

    for (it = lit->notHasIds_Score.begin(); it != lit->notHasIds_Score.end(); it++) {
        cout << "packet: " << it->pktId << " has score(#den to exoun): " << it->score <<" the nodes: " <<endl;

        for (ipIt = it->nodes.begin(); ipIt != it->nodes.end(); ipIt++) {
            cout << *ipIt << " ";
        }
        cout << endl;
    }

}


// gather in notHasIds_Score base all the possible new ids for the encoded pkt combination
// pktId's score is the number of nodes that haven't this
int MyApplication::findPossibleNewId(list<LeecherNode>::iterator lit) {

	list<struct DBase>::iterator it;
	list<struct DBase>::iterator iit;
	list<int>::iterator pit;
	list<int>::iterator ppit;

	if(lit->notHas.size() != 0){
		for(it = lit->notHas.begin(); it != lit->notHas.end(); it++){			// take node of my notHas base
			for(pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++){		// take each missing id from its list
				struct idScore ct;
				if( checkIfExist(*pit, lit->notHasIds_Score)){					// check if we have compute the score of this id
					continue;													// if yes go to next id of this node or next node
				}
				ct.pktId = *pit;
				ct.nodes.push_back(it->nodeAddr);								// nodeAddr of node that we check its id
				ct.score = ct.nodes.size();

				// compare all ids of nodes with *pit and score++ if another node hasn't it
				if( it != lit->notHas.end() ){
					iit = it;
					for(iit++; iit != lit->notHas.end(); iit++){
						for( ppit = iit->pktIds.begin(); ppit != iit->pktIds.end(); ppit++){
							if( ct.pktId == *ppit) {
								ct.nodes.push_back(iit->nodeAddr);
								ct.score = ct.nodes.size();
								break;
							}
							if( ct.pktId < *ppit ){								// the ids are sorted
								break;
							}
						}
					}
				}
				lit->notHasIds_Score.push_back(ct);		// push back in Base the id, its score ant the nodes is missing from
			}
		}
		return 1;	// notHas not empty
	}
	else{
		return 0;	// notHas empty
	}
}

// to every pktId of notHasId_Score corresponds a list of nodes
// for this list, find the common ids among them and their score -in how many nodes you find this id-
int MyApplication::constructCommonIds_ScoreBase(list<LeecherNode>::iterator lit, list<IPvXAddress> nodes, int scoreNotHas){
	cout<< endl;

	list<struct DBase>::iterator it;
	list<struct DBase>::iterator iit;

	list<int>::iterator pit;
	list<int>::iterator ppit;

	list<IPvXAddress>::iterator nodesIt;
	list<IPvXAddress>::iterator nodesIIt;
	unsigned int i;
	int flag=0;

	cout << " list of notHasIdsScore nodes: "<< endl;
	for(nodesIt = nodes.begin(); nodesIt != nodes.end(); nodesIt++){ cout<< *nodesIt<< " " ;} cout << endl;

	if(lit->Common.size() != 0){
		for(nodesIt = nodes.begin(); nodesIt != nodes.end(); nodesIt++){				// take each node that correspond to the score nodes list
			//cout<< " search "<< *nodesIt << endl;
			for(i=0,it = lit->Common.begin(); it != lit->Common.end(); it++,i++){ 				// find now this node in CommonList
				if(*nodesIt == it->nodeAddr){
					//cout<< "we found " << *nodesIt << " in common list" << endl;
					break;
				}
			}
			if(i == lit->Common.size()){												// *nodesIt there isn't in Common base
				//cout<<"we didn't find node " << *nodesIt<< endl;
				continue;
			}
			//cout<< endl << "find the ids of "<< it->nodeAddr<< endl;
			// work for it->nodeAddr
			for(pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++){				// explore the ids of Common Base for it->nodeAddr
				if( checkIfExist(*pit, lit->commonIds_Score) ){							// check if we have this id score already in commonIds_Score
					//cout<<"we have already its score"<< endl;
					continue;
				}
				flag++;
				struct idScore ct;
				ct.pktId = *pit;
				ct.score = 1;
				ct.nodes.push_back(it->nodeAddr);
				//cout<< "take this:" << *pit<< endl;
				// try to find ct.pktId in other's in ids common list
				if(nodesIt != nodes.end()){
					nodesIIt = nodesIt;
					for(nodesIIt++; nodesIIt != nodes.end(); nodesIIt++){					// take the next node of the list nodes
						for(iit = lit->Common.begin(); iit != lit->Common.end(); iit++){	// find node in Common list
							if(*nodesIIt == iit->nodeAddr){
								//cout<< "we found " << *nodesIIt << " in common list" << endl;
								break;
							}
						}
						if(iit == lit->Common.end()){ 												// *nodesIIt there isn't in Common base
							//cout<<"we didn't find " << *nodesIIt<< " in Common base" << endl;
							continue;
						}
						for( ppit = iit->pktIds.begin(); ppit != iit->pktIds.end(); ppit++){
							if( ct.pktId == *ppit){											// found the same id, raise its score
								//cout << iit->nodeAddr << " has the same pkt: " << *ppit << endl;
								ct.nodes.push_back(iit->nodeAddr);
								ct.score++;
								break;
							}
							if( ct.pktId < *ppit){											// due to sorted ids list make this check
								//cout<< iit->nodeAddr << " not has this pkt: " << ct.pktId << endl;
								break;
							}
						}
					}
					// checks the score for avoiding exhaustive search
					if((double)ct.score >= metric*(double)scoreNotHas){		// for this ct.pktId, the percentage of nodes that have this in common it's ok
						//cout << "yeahh, ct.score:" <<ct.score<<",metric*scoreNotHas:"<< metric*scoreNotHas<< endl;
						lit->commonIds_Score.push_back(ct);
						return 1;
					}
				}
				lit->commonIds_Score.push_back(ct);
			}
		}
		if(flag!=0){ return 1;}
	}
	return 0;
}

void MyApplication::printCommonIds_ScoreBase(list<LeecherNode>::iterator lit){
	cout<<endl<<"-------printCommonIds_ScoreBase------" << endl;
	list<struct idScore>::iterator it;
	list<IPvXAddress>::iterator ipIt;

    for (it = lit->commonIds_Score.begin(); it != lit->commonIds_Score.end(); it++) {
        cout << "packet: " << it->pktId << " has score: " << it->score <<" the nodes: " << endl;
        for (ipIt = it->nodes.begin(); ipIt != it->nodes.end(); ipIt++) {
            cout << *ipIt << " ";
        }
        cout << endl;
    }
}



// finds common ids of the list of nodes that correspond to the notHasIds_Score base
int MyApplication::commonInXorCombination(list<LeecherNode>::iterator lit){
	cout<< endl;
	list<struct idScore>::iterator it;
	list<struct idScore>::iterator comIt;

	for(it = lit->notHasIds_Score.begin(); it != lit->notHasIds_Score.end(); it++){
		if(constructCommonIds_ScoreBase(lit, it->nodes, it->score)==0){			// find how many nodes of node list that correspond to the notHas pktId
			cout<<"Common base is empty"<< endl;
			break;
		}
		sortCommonIds_ScoreBase(lit);
		printCommonIds_ScoreBase(lit);

		comIt = lit->commonIds_Score.begin();
		// use of metric. 0.5 now, -values in (0,1)- percentage of the nodes have common pktId among them, for a specific notHasId
		if((double)comIt->score >= metric*(double)it->score){
			cout<<endl<<"XXXXXXXXX GO TO SEND XOR XXXXXXXXXX"<< endl;
			xorPktsANDSend(it->pktId, comIt->pktId, it->nodes);
			return 1;
		}
		lit->commonIds_Score.clear(); // clear it and built it for the nodes another notHasId
	}
	cout << lit->leecherIp << " NO COMBINATION TO SEND"<< endl;
	return 0;
}

void MyApplication::clear(list<LeecherNode>::iterator lit){
	lit->Common.clear();
	lit->notHas.clear();
	lit->notHasIds_Score.clear();
	lit->commonIds_Score.clear();
	lit->gotInfo = 0;
}
