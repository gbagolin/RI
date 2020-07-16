/*
 * Solver.h
 *
 *  Created on: Aug 4, 2012
 *      Author: vbonnici
 */
/*
Copyright (c) 2014 by Rosalba Giugno

This library contains portions of other open source products covered by separate
licenses. Please see the corresponding source files for specific terms.

RI is provided under the terms of The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SOLVER_H_
#define SOLVER_H_

#include "MatchingMachine.h"
#include "Graph.h"
#include <set>

namespace rilib{

class Solver{
public:
	MatchingMachine& mama;
	Graph& rgraph;
	Graph& qgraph;
	AttributeComparator& nodeComparator;
	AttributeComparator& edgeComparator;
	MatchListener& matchListener;

	long steps;
	long triedcouples;
	long matchedcouples;

public:

	Solver(
			MatchingMachine& _mama,
			Graph& _rgraph,
			Graph& _qgraph,
			AttributeComparator& _nodeComparator,
			AttributeComparator& _edgeComparator,
			MatchListener& _matchListener
			)
			: mama(_mama), rgraph(_rgraph), qgraph(_qgraph), nodeComparator(_nodeComparator), edgeComparator(_edgeComparator), matchListener(_matchListener){

		steps = 0;
		triedcouples = 0;
		matchedcouples = 0;
	}

	virtual ~Solver(){}


	void solve(){
		//indice nodi grafo target
		int ii;
		//numero nodi grafo pattern
		int nof_sn 						        = mama.nof_sn;
		//attributi dei nodi grafo pattern(etichette)
		void** nodes_attrs 				    = mama.nodes_attrs;				//indexed by state_id
		//grado dei nodi grafo pattern
		int* edges_sizes 				      = mama.edges_sizes;				//indexed by state_id
		//oggetto degli archi, contiene target, source e attribute dell'arco, grafo pattern
		MaMaEdge** edges 				      = mama.edges;					//indexed by state_id
		//dato un indice di un nodo, restituisce la sua posizione nell'ordinamento del grafo pattern. 
		int* map_node_to_state 			  = mama.map_node_to_state;			//indexed by node_id
		//data una posizione dell'ordinamento, restituisce l'indice del nodo del grafo pattern. 
		int* map_state_to_node 			  = mama.map_state_to_node;			//indexed by state_id
		//dato l'indice di un nodo restituisce la posizione del padre nell'ordinamento del grafo pattern.
		int* parent_state 				    = mama.parent_state;			//indexed by state_id
		//dato l'indice di un nodo restituisce il tipo del padre 
		//(1. se è NULL allora è il primo nodo. 
		// 2. se è IN allora l'arco che va dal padre al figlio è entrante
		// 3. se è OUT allora l'arco che va dal padre al figlio è uscente)
		MAMA_PARENTTYPE* parent_type 	= mama.parent_type;				//indexed by state id

		//Lista di tutti i nodi del grafo target. 
		int* listAllRef = new int[rgraph.nof_nodes];
		//Inizializza la lista con i nodi del grafo target. 
		for(ii=0; ii<rgraph.nof_nodes; ii++)
			listAllRef[ii] = ii;

		//Matrice dove per ogni nodo pattern si tengono tutti i nodi target dei possibili candidati. 
		int** candidates = new int*[nof_sn];							//indexed by state_id
		//Lista di possibili candidati che devono matchare un nodo si del grafo pattern. 
		int* candidatesIT = new int[nof_sn];							//indexed by state_id
		//Quantità di possibili elementi di match per ogni nodo pattern. 
		int* candidatesSize = new int[nof_sn];							//indexed by state_id
		//Contiene i candidati soluzione. 
		//(solution[0] conterrà il nodo target compatibile con il nodo in posizione 0 nella lista ordinata del grafo pattern.)
		int* solution = new int[nof_sn];								//indexed by state_id
		for(ii=0; ii<nof_sn; ii++){
			solution[ii] = -1;
		}
		//Array di set di interi di dimensione pari al numero di nodi del grafo pattern. 
		//Contiene i nodi target matchati.  
		std:set<int>* cmatched = new std::set<int>[nof_sn];
		//Array che tiene traccia dei nodi target matchati. 
		bool* matched = (bool*) calloc(rgraph.nof_nodes, sizeof(bool));		//indexed by node_id
		//Accoppio tutti i nodi target con il primo nodo pattern. 
		candidates[0] = listAllRef;
		//Viene specificata la quantità di possibili candidati del nodo 0. 
		candidatesSize[0] = rgraph.nof_nodes;
		//L'indice dei candidati. 
		candidatesIT[0] = -1;

		//controllo del back-tracking. 
		int psi = -1;
		//indice del nodo pattern nell'ordine.
		int si = 0;
		//indice del nodo target che matcha si.
		int ci = -1;
		//indice del nodo pattern successivo.
		int sip1;

		while(si != -1){
			//steps++;
			//se un back-tracking è avvenuto, viene liberato il precedente match non compatibile. 
			if(psi >= si){
				matched[solution[si]] = false;
			}
			//reset candidato target. 
			ci = -1;

			//passo al prossimo candidato target forse compatibile con si.
			candidatesIT[si]++;

			//finchè non ne trova uno effettivamente compatibile 
			//TODO: questo è parallelizzabile. 
			while(candidatesIT[si] < candidatesSize[si]){
				//triedcouples++;
				//passo il nodo target che voglio considerare. 
				ci = candidates[si][candidatesIT[si]];
				solution[si] = ci;

//				std::cout<<"[ "<<map_state_to_node[si]<<" , "<<ci<<" ]\n";
//				if(matched[ci]) std::cout<<"fails on alldiff\n";
//				if(!nodeCheck(si,ci, map_state_to_node)) std::cout<<"fails on node label\n";
//				if(!(edgesCheck(si, ci, solution, matched))) std::cout<<"fails on edges \n";

				//condizioni di sub-isomorfismo. se soddisfatto ho trovato il nodo compatibile.  
				if(	  (!matched[ci])
					  && (cmatched[si].find(ci)==cmatched[si].end())
				      && nodeCheck(si,ci, map_state_to_node)
				      && edgesCheck(si, ci, solution, matched)
				            ){
					break;
				}
				else{
					ci = -1;
				}
				//non ho trovato il candidato, passo al prossimo 
				candidatesIT[si]++;
			}
			//se non ho trovato nulla, effettuo un back-tracking liberando il set cmatched. 
			if(ci == -1){
				psi = si;
				cmatched[si].clear();
				si--;
			}
			//se ho trovato un nodo che matcha, lo inserisco nel cmatched. 
			else{
				cmatched[si].insert(ci);
				matchedcouples++;
				//se il nodo pattern è l'ultimo, allora ho finito. 
				if(si == nof_sn -1){
					matchListener.match(nof_sn, map_state_to_node, solution);
#ifdef FIRST_MATCH_ONLY
					si = -1;
#endif
					psi = si;
				}
				//se il nodo pattern non è l'ultimo passo al successivo. 
				else{
					//flagga come matchato il nodo target compatibile. 
					matched[solution[si]] = true;
					//si passa al figlio del nodo pattern
					sip1 = si+1; 
					//se il nodo pattern è il primo
					if(parent_type[sip1] == PARENTTYPE_NULL){
						//i candidati del nodo pattern successivo sono tutti i nodi target. 
						candidates[sip1] = listAllRef;
						//quantità di candidati del nodo successivo. 
						candidatesSize[sip1] = rgraph.nof_nodes;
					}
					else{
						//se si non è il primo nodo e il padre ha un arco in ingresso 
						if(parent_type[sip1] == PARENTTYPE_IN){
							//viene passata la lista di candidati, che è la lista di adiacenza del padre. 
							candidates[sip1] = rgraph.in_adj_list[solution[parent_state[sip1]]];
							candidatesSize[sip1] = rgraph.in_adj_sizes[solution[parent_state[sip1]]];
						}
						else{//(parent_type[sip1] == MAMA_PARENTTYPE::PARENTTYPE_OUT)
							//se il padre ha l'arco in uscita. per il resto leggi sopra. 
							candidates[sip1] = rgraph.out_adj_list[solution[parent_state[sip1]]];
							candidatesSize[sip1] = rgraph.out_adj_sizes[solution[parent_state[sip1]]];
						}
					}
					//resetta l'indice dei candidati. 
					candidatesIT[si +1] = -1;

					psi = si;
					//passo al prossimo nodo pattern. 
					si++;
				}
			}
		}
    
    // memory cleanup
    free(matched);
    delete[] cmatched;
    delete[] solution;
    delete[] candidatesSize;
    delete[] candidatesIT;
    delete[] candidates;
    delete[] listAllRef;
	}


	virtual bool nodeCheck(int si, int ci, int* map_state_to_node)=0;
	virtual bool edgesCheck(int si, int ci, int* solution, bool* matched)=0;


};

}


#endif /* SOLVER_H_ */
