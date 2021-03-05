/*
 *
 * to be revised by Jalal
 *
 */
#ifndef TRIE_APP_H_
#define TRIE_APP_H_

#include "../system/worker.h"
#include "../system/task.h"
#include <fstream>
#include <assert.h>
#include "trie.h"
#include "graph.h"
#include <climits>


float TIME_THRESHOLD; // if running time >= TIME_THRESHOLD, split into subtasks
size_t spawned_num = 0;
mutex spawned_num_mtx;

Graph global_g;
VERTEX *global_pvertices;
int num_of_cands;

Trie<char> *trie;

struct ContextValue
{
	int round;
	Graph split_g; //for time delay split

	//for Expand() input
	int nclique_size;
	int num_of_cands;
	int num_of_tail_vertices;
	VERTEX *pvertices;

	//Reduce-Mem: set round to distinguish spawned task and splitted task
	ContextValue(){
		round = -1;
	}

	~ContextValue(){
		if(round != -1){
			split_g.DestroySplitGraph();
		}
		if(pvertices != NULL)
			delete []pvertices;
	}
};


obinstream & operator>>(obinstream & m, ContextValue & c)
{
	m >> c.round;
	if(c.round == 2)
		m >> c.split_g;
	m >> c.nclique_size;
	m >> c.num_of_cands;
	m >> c.num_of_tail_vertices;
	int num_of_vertices = c.nclique_size + c.num_of_cands + c.num_of_tail_vertices;
	c.pvertices = new VERTEX[num_of_vertices];
	for(int i = 0; i < num_of_vertices; i++)
		m >> c.pvertices[i];

    return m;
}

ibinstream & operator<<(ibinstream & m, const ContextValue & c)
{
	m << c.round;
	if(c.round == 2)
		m << c.split_g;
	m << c.nclique_size;
	m << c.num_of_cands;
	m << c.num_of_tail_vertices;
	int num_of_vertices = c.nclique_size + c.num_of_cands + c.num_of_tail_vertices;
	for(int i = 0; i < num_of_vertices; i++)
		m << c.pvertices[i];

    return m;
}

ofbinstream & operator>>(ofbinstream & m, ContextValue & c)
{
	m >> c.round;
	if(c.round == 2)
		m >> c.split_g;
	m >> c.nclique_size;
	m >> c.num_of_cands;
	m >> c.num_of_tail_vertices;
	int num_of_vertices = c.nclique_size + c.num_of_cands + c.num_of_tail_vertices;
	c.pvertices = new VERTEX[num_of_vertices];
	for(int i = 0; i < num_of_vertices; i++)
		m >> c.pvertices[i];

    return m;
}

ifbinstream & operator<<(ifbinstream & m, const ContextValue & c)
{
	m << c.round;
	if(c.round == 2)
		m << c.split_g;
	m << c.nclique_size;
	m << c.num_of_cands;
	m << c.num_of_tail_vertices;
	int num_of_vertices = c.nclique_size + c.num_of_cands + c.num_of_tail_vertices;
	for(int i = 0; i < num_of_vertices; i++)
		m << c.pvertices[i];

    return m;
}

//-------------------------------------------
// check if input file is empty
bool empty(ifstream &pFile)
{
    return pFile.peek() == ifstream::traits_type::eof();
}

typedef Task<ContextValue> QCTask;
bool setup_task(QCTask *task, int root_id)
{
//	int root_id = global_pvertices[root_index].nvertex_no;
//	set<int> id_set;
//	id_set.insert(root_id);
	//temp_mark[v_id] is true if v is valid candidate
	vector<bool> temp_mark(global_g.mnum_of_vertices, false);
	temp_mark[root_id] = true;
	vector<int> id_array;
	id_array.push_back(root_id);

	//set degree for pnew_vertices
	int* root_2hop = global_g.mpplvl2_nbs[root_id];
	for(int i=1; i<=root_2hop[0]; i++)
	{
		//only pull vertex which is post-located of v in v's 2hop nbs
		int nb_id = root_2hop[i];
		if(nb_id > root_id)
		{
//			id_set.insert(nb_id);
			temp_mark[nb_id] = true;
			id_array.push_back(nb_id);
		}
	}
	//check min_size before task spwan
	if(id_array.size() < gnmin_size) return false;

	task->context.pvertices = new VERTEX[id_array.size()];
	VERTEX *pnew_vertices = task->context.pvertices;

	//calculate the degree for each v in new pvertex

	pnew_vertices[0].nvertex_no = root_id;
	pnew_vertices[0].bis_cand = true;
	pnew_vertices[0].bto_be_extended = true;
	pnew_vertices[0].nclique_deg = 0;
	pnew_vertices[0].nlvl2_nbs = id_array.size()-1;

	//prepare ncand_deg for spawned vertex
	int count = 0;
	int* root_1hop = global_g.mppadj_lists[root_id];
	for(int j=1; j<=root_1hop[0]; j++)
	{
		int hop1_id = root_1hop[j];
//		if(id_set.find(hop1_id) != id_set.end())//use vector bool
		if(temp_mark[hop1_id])
			count++;
	}
	pnew_vertices[0].ncand_deg = count;

	for(int i=1; i<id_array.size(); i++)
	{
		int v_id = id_array[i];
		pnew_vertices[i].nvertex_no = v_id;
		pnew_vertices[i].bis_cand = true;
		pnew_vertices[i].bto_be_extended = false; //only expand spawn vertex
		pnew_vertices[i].nclique_deg = 0;
		//prepare ncand_deg
		count = 0;
		int* nb_1hop = global_g.mppadj_lists[v_id];
		for(int j=1; j<=nb_1hop[0]; j++)
		{
			int hop1_id = nb_1hop[j];
//			if(id_set.find(hop1_id) != id_set.end())
			if(temp_mark[hop1_id])
				count++;
		}
		pnew_vertices[i].ncand_deg = count;

		//prepare nlvl2_nbs
		count = 0;
		//id_set -> vector bool
		int* nb_2hop = global_g.mpplvl2_nbs[v_id];
		for(int j=1; j<=nb_2hop[0]; j++)
		{
			int hop2_id = nb_2hop[j];
//			if(id_set.find(hop2_id) != id_set.end())
			if(temp_mark[hop2_id])
				count++;
		}
		pnew_vertices[i].nlvl2_nbs = count;
	}
	temp_mark.clear();

	task->context.nclique_size = 0;
	task->context.num_of_cands = id_array.size();
	task->context.num_of_tail_vertices = 0;
	return true;
}

class QCComper : public Comper<QCTask, int>
{
public:

	int Expand(VERTEX *pvertices, int nclique_size, int num_of_cands, int num_of_tail_vertices, FILE *gfpout, Graph& gograph)
	{
		VERTEX *pnew_vertices, *pnew_cands, *pclique;
		int num_of_vertices, num_of_new_cands, i, j, num_of_new_tail_vertices, nmin_deg;
		bool bis_subsumed, blook_succeed, bgen_new_lvl2nbs;
		int nisvalid, nremoved_vertices;
		int nsuperclique_size, nmax_clique_size, nnew_clique_size;
		struct timeb cur_time;
		double drun_time;
		CLQ_STAT one_clq_stat;

		num_of_vertices = nclique_size+num_of_cands+num_of_tail_vertices;
		pnew_vertices = new VERTEX[num_of_vertices];
		pclique = new VERTEX[nclique_size+1];
		nmax_clique_size = 0;

		for(i=nclique_size;i<nclique_size+num_of_cands && pvertices[i].bis_cand && pvertices[i].bto_be_extended;i++) // not iterating covered vertices (segment 3)
		{
			gograph.VerifyVertices(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, false);
			if(i>nclique_size)
				nisvalid = gograph.RemoveCandVertex(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, i);
			else
				nisvalid = 1;
			if(nisvalid==-1)
				break;
			else if(nisvalid==1 && nclique_size+1<gnmax_size)
			{
				if(i<nclique_size+num_of_cands-1)
				{
					blook_succeed = gograph.Lookahead(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, i, pnew_vertices, gfpout);
					if(blook_succeed)
					{
						if(nmax_clique_size<nclique_size+(nclique_size+num_of_cands-i))
							nmax_clique_size = nclique_size+(nclique_size+num_of_cands-i);
						break;
					}
				}

				if(gdmin_deg_ratio==1)
					num_of_new_cands = gograph.AddOneVertex(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, i, true, pnew_vertices, num_of_new_tail_vertices, &one_clq_stat);
				else
					num_of_new_cands = gograph.AddOneVertex(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, i, false, pnew_vertices, num_of_new_tail_vertices, &one_clq_stat);
				nnew_clique_size = nclique_size+1;
				if(num_of_new_cands>0)
					gograph.CrtcVtxPrune(pnew_vertices, nnew_clique_size, num_of_new_cands, num_of_new_tail_vertices, pclique, &one_clq_stat);

				bis_subsumed = false;
				pnew_cands = &pnew_vertices[nnew_clique_size];
				if(num_of_new_cands>0)
				{
					ftime(&cur_time);
					drun_time = cur_time.time-gograph.gtime_start.time+(double)(cur_time.millitm-gograph.gtime_start.millitm)/1000;
					if(drun_time < TIME_THRESHOLD)
					{
						bgen_new_lvl2nbs = gograph.GenCondGraph(pnew_vertices, nnew_clique_size, num_of_new_cands, num_of_new_tail_vertices);
						nremoved_vertices = gograph.ReduceCands(pnew_vertices, nnew_clique_size, num_of_new_cands+num_of_new_tail_vertices, bis_subsumed);

						if(num_of_new_cands-nremoved_vertices>0) // there is still some candidate left for expansion
							nsuperclique_size = Expand(pnew_vertices, nnew_clique_size, num_of_new_cands, num_of_new_tail_vertices, gfpout, gograph);
						else
							nsuperclique_size = 0;
						if(bgen_new_lvl2nbs)
							gograph.DelCondGraph();
					} else {
						//to split
						QCTask * t = new QCTask;
						t->context.split_g.mnum_of_vertices = gograph.mnum_of_vertices;
						t->context.split_g.mblvl2_flag = gograph.mblvl2_flag;
						t->context.pvertices = NULL; //for ~ContextValue()
						gograph.ForceGenCondGraph(pnew_vertices, nnew_clique_size, num_of_new_cands, num_of_new_tail_vertices, t->context.split_g);
						nremoved_vertices = gograph.ReduceCands(pnew_vertices, nnew_clique_size, num_of_new_cands+num_of_new_tail_vertices, bis_subsumed);
						if(num_of_new_cands-nremoved_vertices>0) // there is still some candidate left for expansion
						{

							//prepare input of Expand()
							t->context.pvertices = new VERTEX[num_of_vertices];
							memcpy(t->context.pvertices, pnew_vertices, sizeof(VERTEX)*(num_of_vertices));
							t->context.nclique_size = nnew_clique_size;
							t->context.num_of_cands = num_of_new_cands;
							t->context.num_of_tail_vertices = num_of_new_tail_vertices;
							//Reduce-Mem: set round = 2 for splitted tasks
							t->context.round = 2;
							add_task(t);
						} else {
//							Graph& split_g =  t->context.split_g;
//							for(i=0;i<split_g.mnum_of_vertices;i++)
//								delete []split_g.mpplvl2_nbs[i];
//							delete []split_g.mpplvl2_nbs;
//
//							for(i=0;i<split_g.mnum_of_vertices;i++)
//								delete []split_g.mppadj_lists[i];
//							delete []split_g.mppadj_lists;
							delete t;
						}

						//alway need to check current task when split happen
						nsuperclique_size = 0;

//						if(bgen_new_lvl2nbs)
//							gograph.DelCondGraph();
					}
				}
				else
					nsuperclique_size = 0;

				//check whether current set is a QC

				if(nsuperclique_size==0 && !bis_subsumed)
				{
					if(nnew_clique_size>=gnmin_size)
					{
						nmin_deg = gograph.GetMinDeg(nnew_clique_size);
						for(j=0;j<nnew_clique_size;j++)
						{
							if(pnew_vertices[j].nclique_deg<nmin_deg)
								break;
						}
						if(j>=nnew_clique_size)
						{
							if(gdmin_deg_ratio<1)
								num_of_new_tail_vertices = 0;
							else if(num_of_new_tail_vertices==0)
								num_of_new_tail_vertices = gograph.GenTailVertices(pvertices, nclique_size, num_of_cands, num_of_tail_vertices, i, pnew_vertices, nnew_clique_size);
							gograph.OutputOneClique(pnew_vertices, nnew_clique_size, num_of_new_tail_vertices, gfpout);
							if(nmax_clique_size<nnew_clique_size)
								nmax_clique_size = nnew_clique_size;
						}
						else if(nnew_clique_size>nclique_size+1 && nclique_size+1>=gnmin_size)
						{
							nmin_deg = gograph.GetMinDeg(nclique_size+1);
							for(j=0;j<=nclique_size;j++)
							{
								if(pclique[j].nclique_deg<nmin_deg)
									break;
							}
							if(j>nclique_size)
							{
								memcpy(pnew_vertices, pclique, sizeof(VERTEX)*(nclique_size+1));
								num_of_new_tail_vertices = 0;
								gograph.OutputOneClique(pnew_vertices, nclique_size+1, num_of_new_tail_vertices, gfpout);
								if(nmax_clique_size<nclique_size+1)
									nmax_clique_size = nclique_size+1;
							}
						}
					}
				}
				else if(nsuperclique_size>0)
				{
					if(nmax_clique_size<nsuperclique_size)
						nmax_clique_size = nsuperclique_size;
				}
				else if(nmax_clique_size<nnew_clique_size)
					nmax_clique_size = nnew_clique_size;
			}
		}
		delete []pnew_vertices;
		delete []pclique;

	//	if(num_of_cands>=10 && nmax_clique_size==0 && nisvalid==1)
	//		printf("stop\n");

		return nmax_clique_size;
	}

	virtual bool task_spawn(int &data) // why when add const to arguments, it will not reach this method !!!
	{
		QCTask *task = new QCTask();
		if(global_pvertices[data].bto_be_extended)
		{
			if(setup_task(task, data))
			{
				//Reduce-Mem: set round = 1 for spawned task
				task->context.round = 1;
				return add_task(task);
			}

		}
		//clear global graph when all tasks are spawned
		spawned_num_mtx.lock();
		spawned_num++;
		if(spawned_num == num_of_cands)
		{
			//Guimu-condense: global_g's graph struture is changed when compress
			global_g.DestroySplitGraph();
			delete []global_g.gpvertex_order_map;
			delete []global_g.gptemp_array;
			delete []global_pvertices;
		}
		spawned_num_mtx.unlock();

		return false;
	}

    virtual void compute(ContextT &context)
    {
//        // Insert to Trie.
//        vector<char> seq(context.begin(), context.end()); // string to vector<char>
//        trie->insert(seq);
    	Graph& split_g = context.split_g;
    	if(context.round == 1)
    	{
    		//Reduce-Mem: if it is spawned task, setup condensed graph for it
			split_g.mnum_of_vertices = global_g.mnum_of_vertices;
			split_g.mblvl2_flag = global_g.mblvl2_flag;
			global_g.ForceGenCondGraph1(context.pvertices, 0, context.num_of_cands, 0, split_g);

			//Reduce-Mem: clear global graph when all tasks are spawned
			spawned_num_mtx.lock();
			spawned_num++;
			if(spawned_num == num_of_cands)
			{
				global_g.DestroySplitGraph();
				delete []global_g.gpvertex_order_map;
				delete []global_g.gptemp_array;
				delete []global_pvertices;
			}
			spawned_num_mtx.unlock();
    	}

    	split_g.SetupGraph(context.nclique_size, context.num_of_cands, context.num_of_tail_vertices);
		ftime(&split_g.gtime_start);
		Expand(context.pvertices, context.nclique_size, context.num_of_cands, context.num_of_tail_vertices, gfpout, split_g);

		split_g.ClearGraph(); //delete graph's variable

    }

	virtual bool is_bigTask(QCTask *task)
	{
		if (task->context.num_of_cands > BIGTASK_THRESHOLD)
		{
			return true;
		}
		return false;
	}

};

class QCWorker : public Worker<QCComper>
{
public:
    QCWorker(int num_compers) : Worker(num_compers)
    {
        trie = new Trie<char>();
    }

    ~QCWorker()
    {
    	delete []index2id;
    }

    virtual bool task_spawn(int &data)
	{
		QCTask *task = new QCTask();
		if(global_pvertices[data].bto_be_extended)
		{
			if(setup_task(task, data))
			{
				//Reduce-Mem: set round = 1 for spawned task
				task->context.round = 1;
				return add_task(task);
			}
		}

		//clear global graph when all tasks are spawned
		spawned_num_mtx.lock();
		spawned_num++;
		if(spawned_num == num_of_cands)
		{
			global_g.DestroySplitGraph();
			delete []global_g.gpvertex_order_map;
			delete []global_g.gptemp_array;
			delete []global_pvertices;
		}
		spawned_num_mtx.unlock();
		return false;
	}

    virtual void load_data(char* file_path)
    {
        //vector<DataT> *data_array;
        //data_array->push_back()

        global_pvertices = global_g.Cliques(file_path, num_of_cands);
        if(global_pvertices != NULL)
        {
        	for(int i=0; i<num_of_cands; i++)
        	{
				// data_array->push_back(i);
				data_array.push_back(new int(i));
			}
        }
    }

	virtual bool is_bigTask(QCTask *task)
	{
		if (task->context.num_of_cands > BIGTASK_THRESHOLD)
		{
			return true;
		}
		return false;
	}

    // // insert some tasks (equal to task_capacity) into task_queue before spawn compers, so compers have some tasks to work on.
    // void initialize_tasks() // name is not clear, change it. fill_some_tasks()
    // {
    //     // assert(initial_task_queue_capacity <= data_array->size());
    //     size_t _size = min(initial_task_queue_capacity, data_array->size());
    //     for (int i = 0; i < _size; i++)
    //     {
    //         // cout << "data_array->at(i): " << data_array->at(i) << endl;
    //         task_spawn((*data_array)[i]); 
    //     }
    //     // update global_data_array position
    //     position_g = _size; 
    //     log("task_queue size: " + to_string(task_queue->size()));
    //     // cout << "task_queue size: " << task_queue->size() << endl;
    // }
};

#endif /* TRIE_APP_H_ */
