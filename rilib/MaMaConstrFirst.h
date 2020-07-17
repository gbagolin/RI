/*
 * MaMaConstrFirst.h
 *
 *  Created on: Aug 5, 2012
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

#ifndef MAMACONSTRFIRST_H_
#define MAMACONSTRFIRST_H_

#include "Graph.h"


// __global__
// void findFirstElement() {
// 	printf("sono il thread numero: %d", threadIdx.x); 
// }


namespace rilib{

class MaMaConstrFirst : public MatchingMachine{

public:

	MaMaConstrFirst(Graph& query) : MatchingMachine(query){

	}

	virtual void build(Graph& ssg){
		printf("sono dentro a mia mamma"); 
		//findFirstElement<<< 1,1024>>> ();
		//tipo di nodo che sto analizzando, 
		//ns_core = nodo in analisi, ns_cneigh = nodo vicino al nodo core, ns_unv = nodo non ancora visitato. 
		enum NodeFlag {NS_CORE, NS_CNEIGH, NS_UNV};
		//lista di tipi nodo per ogni nodo. 
		NodeFlag* node_flags = new NodeFlag[nof_sn]; 						//indexed by node_id
		//pesi (3) per ogni nodo utili per l'ordinamento. 
		int** weights = new int*[nof_sn];									//indexed by node_id
		//lista di nodi padre. 
		int* t_parent_node = (int*) calloc(nof_sn, sizeof(int));			//indexed by node_id
		//lista dei tipi di nodi padre. 
		MAMA_PARENTTYPE* t_parent_type = new MAMA_PARENTTYPE[nof_sn];		//indexed by node id


		//per ogni nodo
		for(int i=0; i<nof_sn; i++){
			//setto come unvisited il nodo i
			node_flags[i] = NS_UNV;
			//alloco lo spazio per i pesi e li inizializzo. 
			weights[i] = new int[3];
			weights[i][0] = 0;
			weights[i][1] = 0;
			//weights[i][2] = ssg.out_adj_sizes[i] + ssg.in_adj_sizes[i];
			weights[i][2] = ssg.out_adj_sizes[i] + ssg.in_adj_sizes[i];
			//il padre e il tipo è ignoto. 
			t_parent_node[i] = -1;
			t_parent_type[i] = PARENTTYPE_NULL;
		}



		//l'indice del nodo del grafo che sto analizzando. 
		int si = 0;
		//indice del nodo con weights massimo. 
		int n;
		//nIT indice iteratore per trovare il nodo candidato. 
		//ni rappresenta il nodo vicino al nodo n
		int nIT; int ni;
		int nnIT; int nni;
		//nqueueL rappresenta il numero di elementi inseriti nella lista ordinata. 
		//nqueueR rappresenta la quantità di possibili candidati da inserire nella lista. 
		int nqueueL = 0, nqueueR = 0;
		int maxi, maxv;
		int tmp;

		//per ogni nodo si
		while(si < nof_sn){
			//TODO: provare a parallelizzare questo if. che Gesù ce la mandi buona. 
			//se si è alla prima iterazione
			if(nqueueL == nqueueR){
				//if queue is empty....
				maxi = -1;
				maxv = -1;
				nIT = 0;
				//cerco il nodo di grado massimo
				//TODO: PARALLELIZZARE QUESTO. 
				while(nIT < nof_sn){
					if(node_flags[nIT]==NS_UNV &&  weights[nIT][2] > maxv){
						maxv = weights[nIT][2];
						maxi = nIT;
					}
					nIT++;
				}
				//assegno alla lista ordinata in posizione si il nodo maxi.
				map_state_to_node[si] = maxi;
				//viceversa, guarda sopra. 
				map_node_to_state[maxi] = si; 					  
				t_parent_type[maxi] = PARENTTYPE_NULL;
				t_parent_node[maxi] = -1;
				//incremento il numero di possibili candidati. 
				nqueueR++;
				
				n = maxi;
				nIT = 0;
				//per ogni vicino del nodo con peso massimo 
				//TODO: PARALLELIZZARE QUESTI
				while(nIT < ssg.out_adj_sizes[n]){
					ni = ssg.out_adj_list[n][nIT];
					if(ni != n){
						weights[ni][1]++;
					}
					nIT++;
				}
				//la stessa cosa. 
				while(nIT < ssg.in_adj_sizes[n]){
					ni = ssg.in_adj_list[n][nIT];
					if(ni != n){
						weights[ni][1]++;
					}
					nIT++;
				}
			}
			
			//verrà eseguito dalla seconda iterazione. 
			if(nqueueL != nqueueR-1){
				//l'indice del nodo massimo prende l'ultimo elemento aggiunto alla lista ordinata. 
				maxi = nqueueL;
				//per ogni elemento presente nei candidati
				for(int mi=maxi+1; mi<nqueueR; mi++){
					//cerco un nodo con peso maggiore
					if(wcompare(map_state_to_node[mi], map_state_to_node[maxi], weights) < 0){
						maxi = mi;
					}
				}
				//swap tra due nodi nella lista ordinata. 
				//indice ultimo nodo inserito nella lista ordinata. 
				tmp = map_state_to_node[nqueueL];
				//aggiorno il valore dell'ultimo elemento della lista ordinata. 
				map_state_to_node[nqueueL] = map_state_to_node[maxi];
				//scambio. 
				map_state_to_node[maxi] = tmp;
			}
			//prende il valore massimo. 
			n = map_state_to_node[si];
			map_node_to_state[n] = si;

			//move queue left limit
			nqueueL++;
			//update nodes' flags & weights
			node_flags[n] = NS_CORE;
			nIT = 0;
			//per ogni vicino dell'ultimo nodo inserito nella lista ordinata n: 
			while(nIT < ssg.out_adj_sizes[n]){
				//prende il nodo vicino di n. 
				ni = ssg.out_adj_list[n][nIT];
				if(ni != n){
					weights[ni][0]++;
					weights[ni][1]--;
					//se il vicino non è già stato visitato 
					if(node_flags[ni] == NS_UNV){
						//assegno l'etichetta neigh. 
						node_flags[ni] = NS_CNEIGH;
						//il padre di ni è n. 
						t_parent_node[ni] = n;
//						if(nIT < ssg.out_adj_sizes[n])
						//tipo di arco di ni è uscente. 
						t_parent_type[ni] = PARENTTYPE_OUT;
//						else
//							t_parent_type[ni] = PARENTTYPE_IN;
						//add to queue
						//aggiungo ni come possibile candidato. 
						map_state_to_node[nqueueR] = ni;
						map_node_to_state[ni] = nqueueR;
						nqueueR++;
						//aumento il peso dei vicini del nodo che non è ancora entrato nella lista. 
						nnIT = 0;
						while(nnIT < ssg.out_adj_sizes[ni]){
							nni = ssg.out_adj_list[ni][nnIT];
							weights[nni][1]++;
							nnIT++;
						}
					}
				}
				nIT++;
			}
			//come sopra ma con archi entranti al posto che uscenti. 
			nIT = 0;
			while(nIT < ssg.in_adj_sizes[n]){
				ni = ssg.in_adj_list[n][nIT];
				if(ni != n){
					weights[ni][0]++;
					weights[ni][1]--;

					if(node_flags[ni] == NS_UNV){
						node_flags[ni] = NS_CNEIGH;
						t_parent_node[ni] = n;
//						if(nIT < ssg.out_adj_sizes[n])
//							t_parent_type[ni] = PARENTTYPE_OUT;
//						else
							t_parent_type[ni] = PARENTTYPE_IN;
						//add to queue
						map_state_to_node[nqueueR] = ni;
						map_node_to_state[ni] = nqueueR;
						nqueueR++;

						nnIT = 0;
						while(nnIT < ssg.in_adj_sizes[ni]){
							nni = ssg.in_adj_list[ni][nnIT];
							weights[nni][1]++;
							nnIT++;
						}
					}
				}
				nIT++;
			}
			//incremento il nodo da considerare. 
			si++;
		}
		//SEZIONE CREAZIONE ARCHI 
		//numero di archi totali, uscenti, entranti. 
		int e_count,o_e_count,i_e_count; int i;
		//per ogni nodo della lista ordinata 
		for(si = 0; si<nof_sn; si++){
			//prende il nodo in posizione si dalla lista ordinata e lo assegna a n. 
			n = map_state_to_node[si];

			//nodes_attrs[si] = ssg.nodes_attrs[n];
			//se n ha un padre
			if(t_parent_node[n] != -1)
				//indica la posizione, nella lista ordinata, il padre di si. 
				parent_state[si] = map_node_to_state[t_parent_node[n]];
			else
				parent_state[si] = -1;
			//viene assegnato il tipo del padre di si. 
			parent_type[si] = t_parent_type[n];

			//conto il numero di archi entranti e uscenti. 
			e_count = 0;
			o_e_count = 0;
			for(i=0; i<ssg.out_adj_sizes[n]; i++){
				//se il vicino di n, nella lista ordinata, viene prima di si, incremento il numero di archi totali e uscenti. 
				if(map_node_to_state[ssg.out_adj_list[n][i]] < si){
					e_count++;
					o_e_count++;
				}
			}
			i_e_count = 0;
			for(i=0; i<ssg.in_adj_sizes[n]; i++){
				//uguale a sopra, ma totali e entranti. 
				if(map_node_to_state[ssg.in_adj_list[n][i]] < si){
					e_count++;
					i_e_count++;
				}
			}
			//per ogni nodo, gli si assegna il numero di vicini. 
			edges_sizes[si] = e_count;
			o_edges_sizes[si] = o_e_count;
			i_edges_sizes[si] = i_e_count;
			//costruisce gli archi di si. 
			edges[si] = new MaMaEdge[e_count];
			e_count = 0;
			//mappatura ordinata archi uscenti di n o si. 
			for(i=0; i<ssg.out_adj_sizes[n];i++){ 
				if(map_node_to_state[ssg.out_adj_list[n][i]] < si){
					edges[si][e_count].source = map_node_to_state[n];
					edges[si][e_count].target = map_node_to_state[ssg.out_adj_list[n][i]];
					edges[si][e_count].attr = ssg.out_adj_attrs[n][i];
					e_count++;
				}
			}
//			for(i=0; i<ssg.in_adj_sizes[n];i++){
//				if(map_node_to_state[ssg.in_adj_list[n][i]] < si){
//					edges[si][e_count].target = map_node_to_state[n];
//					edges[si][e_count].source = map_node_to_state[ssg.in_adj_list[n][i]];
//					e_count++;
//				}
//			}
			//mappatura ordinata archi entranti di n o si. 
			for(int j=0; j<si; j++){
				int sn = map_state_to_node[j];
				for(i=0; i<ssg.out_adj_sizes[sn]; i++){
					if(ssg.out_adj_list[sn][i] == n){
						edges[si][e_count].source = j;
						edges[si][e_count].target = si;
						edges[si][e_count].attr = ssg.out_adj_attrs[sn][i];
						e_count++;
					}
				}
			}
		}

		delete[] node_flags;
		for(int i=0; i<nof_sn; i++)
			delete[] weights[i];
		delete[] weights;
		free(t_parent_node);
		delete[] t_parent_type;
	}





private:

	int wcompare(int i, int j, int** weights){
		for(int w=0; w<3; w++){
			if(weights[i][w] != weights[j][w]){
				return weights[j][w] - weights[i][w];
			}
		}
		return i-j;
	}

	void increase(int* ns, int* sis, int i, int** weights, int leftLimit){
		int temp;
		while(i>leftLimit &&   ( wcompare(ns[i], ns[i-1], weights) <0 ) ){
			temp = ns[i-1];
			ns[i-1] = ns[i];
			ns[i] = temp;

			temp = sis[ns[i-1]];
			sis[ns[i-1]] = sis[ns[i]];
			sis[ns[i]] = temp;

			i--;
		}
	}
};

}


#endif /* MAMACONSTRFIRST_H_ */
