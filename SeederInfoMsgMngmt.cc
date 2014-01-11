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


// print Common and notHas Base of thisSeeder
void MyApplication::printBases(list<SeederNode>::iterator sit){
    cout<<endl<<"-----------PRINT BASES OF SEEDER------------:" << sit->seederIp << endl;

    list<struct DBase>::iterator it;	// iterator for every neighbor in Base
    list<int>::iterator pit;			// iterator for pktIds of Base

    for (it = sit->Common.begin(); it != sit->Common.end(); it++) {
        cout << "node: " << it->nodeAddr << " hasCommon " << endl;
        for (pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++) {
            cout << *pit << " ";
        }
        cout << endl;
    }
    for (it = sit->notHas.begin(); it != sit->notHas.end(); it++) {
        cout << "node: " << it->nodeAddr << " notHas " << endl;
        for (pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++) {
            cout << *pit << " ";
        }
        cout << endl;
    }

}

// find common ids and ids that neighbor hasn't
void MyApplication::constructBasesOfSeeder(MyMessage *myMsg, list<SeederNode>::iterator sit) {
	cout << endl;

	vector<int> neighborIds;
	vector<int> commonIds;
	vector<int> notHasIds;
	unsigned int i;
	int myId;
	struct DBase ctCom;
	struct DBase ctNh;

	IPvXAddress neighborAddr = myMsg->getSenderAddress().getIp();


//	cout << neighborAddr << " has: ";
	for (i = 0; i < myMsg->getIdsArraySize(); i++) {		// always idsArraySize!=0
		neighborIds.push_back(myMsg->getIds(i));			// copy to neighbor ids
//		cout << neighborIds.at(i) << " ";
	}

//	cout << endl;
//	cout << sit->seederIp << " has: ";
	//for(myId = 1; myId <= numPktFile; myId++){cout << myId << " "; } cout << endl; // for debug

	ctCom.nodeAddr = neighborAddr;
	ctNh.nodeAddr = neighborAddr;

	for(myId = 1; myId <= numPktFile; myId++){
		for (i = 0; i < myMsg->getIdsArraySize(); i++) {
			if( myId == neighborIds.at(i) ){			// common id
				ctCom.pktIds.push_back(myId);
				//commonIds.push_back(myId);
				break;
			}
			if( myId < neighborIds.at(i) ){				// neighbor hasn't myId
				ctNh.pktIds.push_back(myId);
				//notHasIds.push_back(myId);
				break;
			}
		}
		if (i == neighborIds.size()) { 					// neighbor hasn't myId
			ctNh.pktIds.push_back(myId);
			//notHasIds.push_back(myId); // put myId in neighborIds
		}
	}
	//put in Bases
	//Common Base
	if(ctCom.pktIds.size()!=0){
		sit->Common.push_back(ctCom);
	}
	else{
		cout << " has no common id with " << neighborAddr << endl;
	}
	//NotHas Base
	if(ctNh.pktIds.size()!=0){
		sit->notHas.push_back(ctNh);
	}
	else{
		cout << "all ids of "<< sit->seederIp << " exist at " << neighborAddr << endl;
	}

	printBases(sit);

}


// sort in descending order regarding score
void MyApplication::sortNotHasIds_ScoreBase(list<SeederNode>::iterator sit){

	list<struct idScore>::iterator a;
	list<struct idScore>::iterator b;
	list<struct idScore>::iterator c;
	int i,j;

	// SelectionSort
	a = sit->notHasIds_Score.end();
	for(a--, i = sit->notHasIds_Score.size()-1; i>0; a--,i--){
		c = sit->notHasIds_Score.begin();
		b = sit->notHasIds_Score.begin();
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
void MyApplication::sortCommonIds_ScoreBase(list<SeederNode>::iterator sit){

	list<struct idScore>::iterator a;
	list<struct idScore>::iterator b;
	list<struct idScore>::iterator c;
	int i,j;

	// SelectionSort
	a = sit->commonIds_Score.end();
	for(a--, i = sit->commonIds_Score.size()-1; i>0; a--,i--){
		c = sit->commonIds_Score.begin();
		b = sit->commonIds_Score.begin();
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


void MyApplication::printNotHasIds_Score(list<SeederNode>::iterator sit){
    cout << endl << "print NOTHAS IDS score" << endl;

    list<struct idScore>::iterator it;
    list<IPvXAddress>::iterator ipIt;

    for (it = sit->notHasIds_Score.begin(); it != sit->notHasIds_Score.end(); it++) {
        cout << "packet: " << it->pktId << " has score(#den to exoun): " << it->score <<" the nodes: " <<endl;

        for (ipIt = it->nodes.begin(); ipIt != it->nodes.end(); ipIt++) {
            cout << *ipIt << " ";
        }
        cout << endl;
    }

}


// gather in notHasIds_Score base all the possible new ids for the encoded pkt combination
// pktId's score is the number of nodes that haven't this
int MyApplication::findPossibleNewId(list<SeederNode>::iterator sit) {
	cout << endl;
	list<struct DBase>::iterator it;
	list<struct DBase>::iterator iit;
	list<int>::iterator pit;
	list<int>::iterator ppit;

	if(sit->notHas.size() != 0){
		for(it = sit->notHas.begin(); it != sit->notHas.end(); it++){			// take node of my notHas base
			for(pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++){		// take each missing id from its list
				struct idScore ct;
				if( checkIfExist(*pit, sit->notHasIds_Score)){					// check if we have compute the score of this id, at CommonFuncInfoMsg.cc
					continue;													// if yes go to next id of this node or next node
				}
				ct.pktId = *pit;
				ct.nodes.push_back(it->nodeAddr);
				ct.score = ct.nodes.size();
				// compare all ids of nodes with *pit and score++ if another node hasn't it
				if( it != sit->notHas.end() ){
					iit = it;
					for( iit++; iit != sit->notHas.end(); iit++){
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
				sit->notHasIds_Score.push_back(ct);		// push back in Base the id, its score ant the nodes is missing from
			}
		}
		return 1;
	}
	else{
		return 0;
	}
}

// to every pktId of notHasId_Score corresponds a list of nodes
// for this list, find the common ids among them and their score -in how many nodes you find this id-
int MyApplication::constructCommonIds_ScoreBase(list<SeederNode>::iterator sit, list<IPvXAddress> nodes, int scoreNotHas){
	cout<< endl;

	list<struct DBase>::iterator it;
	list<struct DBase>::iterator iit;

	list<int>::iterator pit;
	list<int>::iterator ppit;

	list<IPvXAddress>::iterator nodesIt;
	list<IPvXAddress>::iterator nodesIIt;

	int flag = 0;
	cout << " list of notHasIdsScore: "<< endl;
	for(nodesIt = nodes.begin(); nodesIt != nodes.end(); nodesIt++){ cout<< *nodesIt<< " " ;} cout << endl;

	if( sit->Common.size()!=0){
		for(nodesIt = nodes.begin(); nodesIt != nodes.end(); nodesIt++){				// take each node that correspond to the score nodes list
			//cout<< " search "<< *nodesIt << endl;
			for(it = sit->Common.begin(); it != sit->Common.end(); it++){ 				// find now this node in CommonList
				if(*nodesIt == it->nodeAddr){
					//cout<< "we found " << *nodesIt << " in common list" << endl;
					break;
				}
			}
			if(it == sit->Common.end()){ 												// *nodesIt there isn't in Common base
				//cout<<"we didn't find " << *nodesIt<< endl;
				continue;
			}
			//cout<< endl << "find the ids of "<< it->nodeAddr<< endl;
			// work for it->nodeAddr
			for(pit = it->pktIds.begin(); pit != it->pktIds.end(); pit++){				// explore the ids of Common Base for it->nodeAddr
				if( checkIfExist(*pit, sit->commonIds_Score) ){							// check if we have this id score already in commonIds_Score
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
						for(iit = sit->Common.begin(); iit != sit->Common.end(); iit++){	// find node in Common list
							if(*nodesIIt == iit->nodeAddr){
								//cout<< "we found " << *nodesIIt << " in common list" << endl;
								break;
							}
						}
						if(iit == sit->Common.end()){ 												// *nodesIIt there isn't in Common base
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
					if((double)ct.score >= metric* (double)scoreNotHas){		// for this ct.pktId, the percentage of nodes that have this in common it's ok
						//cout << "yeahh, ct.score:" <<ct.score<<",metric*scoreNotHas:"<< metric*scoreNotHas<< endl;
						sit->commonIds_Score.push_back(ct);
						return 1;
					}
				}
				//cout<< " the score of "<< ct.pktId <<" is " << ct.score << endl;
				sit->commonIds_Score.push_back(ct);
			}
		}
		if(flag!=0){return 1;}
	}
	return 0;
}



void MyApplication::printCommonIds_ScoreBase(list<SeederNode>::iterator sit){
	cout<<"-------printCommonIds_ScoreBase------" << endl;
	list<struct idScore>::iterator it;
	list<IPvXAddress>::iterator ipIt;
	cout<< "THIS.NODE: " << sit->seederIp << endl;

    for (it = sit->commonIds_Score.begin(); it != sit->commonIds_Score.end(); it++) {
        cout << "packet: " << it->pktId << " has score: " << it->score <<" the nodes: " << endl;
        for (ipIt = it->nodes.begin(); ipIt != it->nodes.end(); ipIt++) {
            cout << *ipIt << " ";
        }
        cout << endl;
    }
}


// finds common ids of the list of nodes that correspond to the notHasIds_Score base
int MyApplication::commonInXorCombination(list<SeederNode>::iterator sit){
	cout<< endl;
	list<struct idScore>::iterator it;
	list<struct idScore>::iterator comIt;

	for(it = sit->notHasIds_Score.begin(); it != sit->notHasIds_Score.end(); it++){
		if(constructCommonIds_ScoreBase(sit, it->nodes, it->score)==0){
			cout<<"Common base is empty"<< endl;
			break;
		}
		// sort the base in descending order
		sortCommonIds_ScoreBase(sit);
		printCommonIds_ScoreBase(sit);

		comIt = sit->commonIds_Score.begin();
		// use of metric. 0.5 now, -values in (0,1)- percentage of the nodes have common pktId among them, for a specific notHasId
		if((double)comIt->score >= metric*(double)it->score){
			cout<<endl<<"XXXXXXXXX GO TO SEND XOR XXXXXXXXXX"<< endl;
			xorPktsANDSend(it->pktId, comIt->pktId, it->nodes);
			return 1;
		}
		sit->commonIds_Score.clear(); // clear it and built it for the nodes another notHasId
	}
	cout << sit->seederIp<< " NO COMBINATION TO SEND"<< endl;
	return 0;
}

void MyApplication::clear(list<SeederNode>::iterator sit){
	sit->Common.clear();
	sit->notHas.clear();
	sit->notHasIds_Score.clear();
	sit->commonIds_Score.clear();
	sit->gotInfo = 0;
}
