/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: temp_corr_calc.c                 Date: 11 May. 2008 1:00 AM   **
** Author: Dylan A. Shell /Muhammad Emad                                **
** Notes:                                                               **
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "temp_corr_calc.h"
#include "clustering.h"
#include <math.h>
#include "utils.h"
//#include <sys/timeb.h>

// Forward decls.
void _temp_corr_init(temp_corr_calc_t * tc);
void _temp_corr_shutdown(temp_corr_calc_t * tc);
void _temp_corr_print_cluster_list(temp_corr_calc_t * tc);

temp_corr_calc_t *temp_corr_calc_create()
{
    temp_corr_calc_t *ret = NULL;

    ret = (temp_corr_calc_t *) malloc(sizeof(temp_corr_calc_t));

    // Set members
    ret->peds = (ped_set_t *) malloc(sizeof(ped_set_t));
    _temp_corr_init(ret);
    return ret;
}

void temp_corr_calc_destroy(temp_corr_calc_t * tc)
{
    assert(tc != NULL);

    int k;
    GList *temp_list;

    for (k = 0; k < (tc->peds)->num_ped; k++)   
    {        
	if ( ((pedestrian_t *)g_list_nth_data((tc->peds)->ped_list, k))->deleted == 0)  //do not delete the pedestrian that is already deleted
	{
		temp_list = ((pedestrian_t *)g_list_nth_data((tc->peds)->ped_list, k))->cluster_list;        	
		clustering_delete_cluster_list(temp_list);
	}
    }
    _temp_corr_shutdown(tc);
    free(tc->peds);
    free(tc);
}

void _temp_corr_init(temp_corr_calc_t * tc)     //add cluster list functionality 
{
    assert(tc != NULL);
    assert(tc->peds != NULL);
    printf("[Temp Corr] Init\n");

    (tc->peds)->ped_list = NULL;
    (tc->peds)->num_ped = 0;

    _temp_corr_print_cluster_list(tc);
    //TODO: Other initialization you need.
}

void _temp_corr_shutdown(temp_corr_calc_t * tc)
{
    assert(tc != NULL);

    if (tc->peds->ped_list != NULL)
    {
        g_list_free(tc->peds->ped_list);
    }

    printf("[Temp Corr] Shutdown\n");
    //TODO : Other cleanup you need
}

int mymax(int x, int y)
{
  return (x > y ? x : y);
}

void temp_corr_calculate(tracking_t * tr, temp_corr_calc_t * tc, cluster_set_t * cs_prev,
                         cluster_set_t * cs_curr)
{
    assert(tc != NULL);
    assert(cs_curr != NULL);
 
    //struct timeb t1;
    //struct timeb t2;
    //ftime(&t1);
    
    int num_ped = (tc->peds)->num_ped;  //num_ped should never reduce other wise change limits for few for loops below     
    int M = cs_curr->numClust;  // No of clusters in current frame
    int ind;
    
    for (ind=tr->start_peds_index;ind<num_ped;ind++)
    {
        (tc->peds)->updated[ind] = 0;
    }

    if (M == 0)
    {
        return;
    }

    int ped_index = -1, ctr_alive=0;
    int new_ped = 0;
    int i,j,ctr=0;
    double interfeetDist = 0.62;//0.4; // more wide stride acceptance could make distract tracker
    double nbrdist = 2.0;   // more distance handles occlusion// measn person can move nbrdist meters while occluded
    double mindistc[M],node_dist[M],clust_dim[M][2];
    double sumx,sumy;
    GList *curr = cs_curr->cluster_list;
    int *latelist;
    static int pt_indx_ct[MAX_PED_VAL];
    int clust_chosen[M],pair[M],ped_chosen[MAX_PED_PER_IT];
    static int deleted[MAX_PED_VAL]; //	this is huge so i made it static
    point_t *clustCentt1;       //centers of max number of curr clusters
    static point_t ped_clust2[MAX_PED_VAL];       //2D pedestrian array that contains pedestrian cluster associations
    static int clust_group[MAX_PED_VAL][MAX_CLUST_PER_PED];          //has its first index for cluster id, second for pedestrian id // NxM // create one slot for each pedestrian where we store its corresp clusters for the current run only
    int **close_pair; 
    GList *pref_table;  

    latelist = (int *)calloc(M, sizeof(int));
    clustCentt1 = (point_t *) calloc(M, sizeof(point_t));

    close_pair = malloc(M * sizeof(int *));
    close_pair[0] = malloc((M * MAX_CLUST_PER_PED) * sizeof(int));  // No of possible clusters X pair
    
    //printf("[temp_corr_calc.c] Initialization Begins\n");
    for (i = 1; i < M; i++)     // initialize pair array
    {
	close_pair[i] = close_pair[0] + i * MAX_CLUST_PER_PED;
    }
  
    for (i = 0; i < M ; i++) 
    {
    	mindistc[i]=100.0;
	node_dist[i]=100.0;
	pair[i]=-1;
    }
     
    GList *temp=NULL,*temp2=NULL;
    pedestrian_t *temp_ped=NULL;
    cluster_t *temp_clust=NULL;

    temp = g_list_first(curr);
    for (i = 0; i < M; i++)     // assign data from objects to local "curr" variables
    {
	temp_clust=(cluster_t *)(temp->data);
	clustCentt1[i].X = temp_clust->mean_X;   
	clustCentt1[i].Y = temp_clust->mean_Y;
	clust_dim[i][0]=temp_clust->dim_X;   
	clust_dim[i][1]=temp_clust->dim_Y;
        latelist[i] = i;
       	clust_chosen[i]=-1;
       	for (j=0; j < 10; j++)
       	{
       		close_pair[i][j]=-1;
       	}
	temp = g_list_next(temp);
    }


    for (i = tr->start_peds_index; i < num_ped; i++)       // assign data from objects to local variables
    {
	temp_ped=(pedestrian_t *)g_list_nth_data((tc->peds)->ped_list, i);	
	pt_indx_ct[i] = temp_ped->clust_card;
        deleted[i] = temp_ped->deleted;
	//printf("[temp_corr_calc.c] tr->people[%d].updated: %d\n",i,tr->people[i].updated);
	
	if (deleted[i] == 0)
	{
		ctr_alive++;
	}
		
	if ( (deleted[i] == 0) && (tr->people[i].first_update == 0))
	{
		tr->people[i].first_update=1;
		sumx=0.0;
		sumy=0.0;
		
		temp=g_list_first(temp_ped->cluster_list);
		for (j = 0; j < pt_indx_ct[i]; j++)
		{
		    temp_clust=(cluster_t *)(temp->data);
		    sumx = sumx + temp_clust->mean_X;   //in ped_clust...only the index is stored..this is not the case with ped_clust2
		    sumy = sumy + temp_clust->mean_Y;
		    //printf("[temp_corr_calc.c] ped_clust2[%d][%d].X: %f\n",i,j,ped_clust2[i][j].X);
		    temp = g_list_next(temp);		
		}
		ped_clust2[i].X = sumx/pt_indx_ct[i];
		ped_clust2[i].Y = sumy/pt_indx_ct[i];
		//printf("[temp_corr_calc.c] ped_clust2[%d].X: %f\n",i,ped_clust2[i].X);
	}else
	{
		if (tr->people[i].life > 0)
		{		
			ped_clust2[i].X = tr->people[i].est_X;
			ped_clust2[i].Y = tr->people[i].est_Y;
			//printf("[temp_corr_calc.c] 2nd condition: ped_clust2[%d].X: %f\n",i,ped_clust2[i].X);
		}else
		{
			if (deleted[i] == 0)   	  
			{
				//printf("[temp_corr_calc.c] too early\n");			    	
				clustering_delete_cluster_list(temp_ped->cluster_list);
			    	temp_ped->clust_card = 0;
       				temp_ped->deleted = 1;
				deleted[i]=1;
				ctr_alive=ctr_alive-1;
				//printf("[temp_corr_calc.c] Ped no. %d deleted\n",i);
			}	 		
		}
	}
    }
	pref_table=NULL;

	for (i=0;i<ctr_alive;i++)
	{
		list* newrow2=(list*) malloc(sizeof(list));
		newrow2->row=NULL;
		pref_table=g_list_append(pref_table, newrow2);
		ped_chosen[i]=0;
	}

    int k,flag_pair=0;
    double mindist=100.0,dist1=0.0,dist2=0.0;
    //printf("[temp_corr_calc.c] Initialization done.\n");
    /////////////////////////////CLUSTER PAIR DETECTION ROUTINE STARTS HERE////////////////////////////////////////////////////////////////////       
	int p_ctr=0; //pair count
	
	for (k = 0; k < M; k++)     // loops through update clusters for pairs
	{		
		mindist=100.0;
		flag_pair=0;
		//printf("[temp_corr_calc.c] cluster area: %f\n",(clust_dim[k][0]*clust_dim[k][1]));
		
		if ((clust_dim[k][0]*clust_dim[k][1]) < 0.09)
		{
			for (j = 0; j < M; j++)   
			{
				if ((k != j) && ((clust_dim[j][0]*clust_dim[j][1]) < 0.09))
				{
					dist1 = sqrt(SQR(clustCentt1[j].X - clustCentt1[k].X) + SQR(clustCentt1[j].Y - clustCentt1[k].Y));  
			
					if ( (dist1 < interfeetDist) && (dist1 < node_dist[j]) && (dist1 < mindist) )
					{
						pair[k]=j;
						mindist=dist1;
						flag_pair=1;
					}  
				}
			}
		
			if (flag_pair == 1)
			{
				node_dist[k]=mindist;		
			}
		}
	}//finished pair finding loop

	for (k = 0; k < M; k++)     // loops through update clusters for pairs
	{
		if ( (pair[k] != -1) && (latelist[k] != -1))
		{
			close_pair[p_ctr][0]=k;
			close_pair[p_ctr][1]=pair[k];
			latelist[k]=-1;
			latelist[ pair[k] ]=-1;
			p_ctr=p_ctr+1;
		}
	}

	for (k = 0; k < M; k++)     // loops through update clusters for singles
	{
		if (latelist[k] != -1)
		{
			close_pair[p_ctr][0]=k;
			latelist[k]=-1;
			p_ctr=p_ctr+1;
		}
	}
        //printf("[temp_corr_calc.c] Safe A\n");        
	/////////////////////////////////////////////END OF CLUSTER PAIR DETECTION ROUTINE/////////////////////////////////////////////////        
        /////////////////////////////////////////////CREATE ASSOCIATION TABLE////////////////////////////////////////////////////////////// 	
	update* temp_elt;
	list* newrow;	
	temp=g_list_first(pref_table);
	//flag_pair=0;
	//printf("[temp_corr_calc.c] Safe A1\n");
	for (ctr = tr->start_peds_index; ctr < num_ped; ctr++)// loop through undeleted peds
	{		
		if (deleted[ctr] == 0)
		{
			//printf("[temp_corr_calc.c] Safe A1.1, flag_pair=%d, ctr_alive=%d, preftable length=%d\n",flag_pair,ctr_alive,g_list_length(pref_table));		
			newrow=(list*)(temp->data);			
			//printf("[temp_corr_calc.c] Safe A1.2\n");		
			for (k = 0; k < p_ctr; k++)     // loops through update cluster pairs		
			{				
				//printf("[temp_corr_calc.c] Safe A2\n");				
				j=0;
				dist1=0.0;
				while (close_pair[k][j] != -1)
				{
					//printf("[temp_corr_calc.c] Safe A2.1, j=%d, k=%d\n",j,k);									
					dist1 = dist1 + sqrt(SQR(clustCentt1[close_pair[k][j]].X - ped_clust2[ctr].X) + SQR(clustCentt1[close_pair[k][j]].Y - ped_clust2[ctr].Y) );
					//printf("[temp_corr_calc.c] Safe A2.2\n");
					j=j+1;
				}
				//printf("[temp_corr_calc.c] Safe A2.3\n");				
				dist2=0.0+j;
				dist1=(dist1/dist2);									

				if (g_list_length(newrow->row) < 1)
				{
					//printf("[temp_corr_calc.c] Safe A4\n");
					update *elt=(update *) malloc(sizeof(update));
					elt->dist_frm_ped=dist1;
					elt->pair_index=k;
					elt->ped_index=ctr;
					newrow->row=g_list_append(newrow->row,elt);
					//printf("[temp_corr_calc.c] safe A5\n");
				}else
				{
					//printf("[temp_corr_calc.c] Safe A6\n");
					temp2=g_list_first(newrow->row);	
					for(i=0;i< g_list_length(newrow->row);i++)
					{
						//printf("[temp_corr_calc.c] Safe A7\n");
						temp_elt=(update*)(temp2->data);
						if ((dist1 <= (temp_elt->dist_frm_ped)) && (i == 0))
						{
							//printf("[temp_corr_calc.c] Safe A8\n");
							update *elt=(update *) malloc(sizeof(update));
							elt->dist_frm_ped=dist1;
							elt->pair_index=k;
							elt->ped_index=ctr;
							newrow->row=g_list_insert(newrow->row,elt, i);
							break;
						}
						else if ( (dist1 > (temp_elt->dist_frm_ped)) && (g_list_length(newrow->row) == 1) )
						{
							//printf("[temp_corr_calc.c] Safe A9\n");
							update *elt=(update *) malloc(sizeof(update));
							elt->dist_frm_ped=dist1;
							elt->pair_index=k;
							elt->ped_index=ctr;
							newrow->row=g_list_insert(newrow->row,elt, i+1);		
							break;
					
						}else if ((dist1 > (temp_elt->dist_frm_ped)) && (i == (g_list_length(newrow->row) -1)))
						{
							//printf("[temp_corr_calc.c] Safe A10\n");
							update *elt=(update *) malloc(sizeof(update));
							elt->dist_frm_ped=dist1;
							elt->pair_index=k;
							elt->ped_index=ctr;
							newrow->row=g_list_insert(newrow->row,elt, i+1);
							break;					
						}else if (i > 0)
						{
							//printf("[temp_corr_calc.c] safe A11\n");
							if ((dist1 > ((update*)g_list_nth_data(newrow->row,i-1))->dist_frm_ped) && (dist1 <= temp_elt->dist_frm_ped))
							{
								update *elt=(update *) malloc(sizeof(update));
								elt->dist_frm_ped=dist1;
								elt->pair_index=k;
								elt->ped_index=ctr;
								newrow->row=g_list_insert(newrow->row,elt, i);
								break;
							}
						}
					//printf("[temp_corr_calc.c] Safe A12\n");
					temp2 = g_list_next(temp2);					
					}//end for
				}//end else				
			} //end cluster pair loop
			//flag_pair++;
			//printf("[temp_corr_calc.c] Safe A12.1\n");
			temp = g_list_next(temp);		
		}//end if deleted pedestrain check
		//printf("[temp_corr_calc.c] Safe A13\n");	
	}
	/////////////////////////////////////////////////////////////////////////////////
	/*int list_ctr=0;
	printf("######################\n");
	ctr=tr->start_peds_index;	
	while (ctr < num_ped) // loop through undeleted peds
	{
		if (deleted[ctr] == 0)
		{
			newrow=(list*)g_list_nth_data(pref_table,list_ctr);
			
			for(i=0;i< g_list_length(newrow->row);i++)
			{
				printf("%f ",((update*)g_list_nth_data(newrow->row,i))->dist_frm_ped);
			}

		list_ctr=list_ctr+1;
		printf("\n");
		}
	ctr=ctr+1;
	}
	printf("######################\n");*/
	//continue work on actual destection now that the ped-cluster distance table is complete
	/////////////////////////////////////////////////////////////////////////////////
        //printf("[temp_corr_calc.c] Safe B\n");        	
	int ped_rec=-1,flag_ped=-1,flag_ped_alive=-1,ped_rec_alive=-1, flag_pair_alive=-1;	
	k=0; 

	while (k < p_ctr) //loop thru clusters
	{		
		mindist=100.0;
		dist2=100.0;	
		flag_ped=-1;		
		ped_rec=-1;
		ped_rec_alive=-1;
		flag_pair=-1;
		flag_pair_alive=-1;
		ctr=0;
		temp=g_list_first(pref_table);
		//printf("[temp_corr_calc.c] Safe B1\n");      	
		for(j=tr->start_peds_index;j<num_ped;j++) //loop thru peds
		{
			if (deleted[j] == 0) 
			{
				if (ped_chosen[ctr] == 0)				
				{				
					update* temp_elt2;				
					//printf("[temp_corr_calc.c] Safe B3\n");			        
					newrow=(list*)(temp->data);
					//printf("[temp_corr_calc.c] Safe B3.1\n");
					temp_elt2=(update*)g_list_nth_data(newrow->row,k);
					//printf("[temp_corr_calc.c] Safe B3.3\n");
					dist1=temp_elt2->dist_frm_ped;
					//printf("[temp_corr_calc.c] Safe B3.4\n");
				
					if ( (dist1 < mindist) && (clust_chosen[temp_elt2->pair_index] == -1) )
					{
						//printf("[temp_corr_calc.c] Safe B3.2\n");					
						flag_pair=temp_elt2->pair_index;//j;
						mindist=dist1;
						flag_ped=ctr;
						ped_rec=temp_elt2->ped_index;
					}

					if ( (dist1 < dist2) && (clust_chosen[temp_elt2->pair_index] == -1) && (tr->people[temp_elt2->ped_index].life >= PED_LIFE) )
					{
						flag_pair_alive=temp_elt2->pair_index;
						dist2=dist1;
						flag_ped_alive=ctr;
						ped_rec_alive=temp_elt2->ped_index;	
					}
					//printf("[temp_corr_calc.c] Safe B4\n");
				}
				temp = g_list_next(temp);				
				ctr=ctr+1;
			}
		}

		if (flag_ped !=-1)
		{			
			if (mindist >= nbrdist) 
			{
				k++;
				ped_chosen[flag_ped]=1;					
			}else
			{
				if (dist2 < interfeetDist) 
				{
					ped_chosen[flag_ped_alive]=1;
					clust_chosen[flag_pair_alive]=ped_rec_alive;
				}else
				{
					ped_chosen[flag_ped]=1;
					clust_chosen[flag_pair]=ped_rec;
				}	
			}	
		}else
		{
			k++;
		}
		//printf("[temp_corr_calc.c] Safe B7\n");
	}
        //printf("[temp_corr_calc.c] Safe C\n");        

	for (k = 0; k < p_ctr; k++)     // loops through update cluster pairs for new peds
	{
		if (clust_chosen[k] > -1)
		{
			j=0;
			while (close_pair[k][j] != -1)
			{
				clust_group[clust_chosen[k]][j] = close_pair[k][j];
				j=j+1;
			}
			pt_indx_ct[clust_chosen[k]] = j;
			(tc->peds)->updated[clust_chosen[k]] = 1;
			//printf("clust_chosen[%d]  : %d\n",k,clust_chosen[k]);
		}else
		{
			new_ped = new_ped + 1;
			ped_index = num_ped + new_ped;
			ctr=0;
			while (close_pair[k][ctr] != -1)
			{
				clust_group[ped_index-1][ctr] = close_pair[k][ctr];
				ctr=ctr+1;
			}
			pt_indx_ct[ped_index-1] = ctr;  // its max 2 because our subjects are bi-ped and data indicates that 2 clusters at max can be associated to a person	
			(tc->peds)->updated[ped_index-1] = 1;
		}
	}
    //printf("[temp_corr_calc.c] Safe D\n");        
    //////////////////////////////////////////END OF BLOB CARRYON/NEW BLOB ROUTINE//////////////////////////////
    //printf("[temp_corr_calc.c] safe H\n");
    //////////////////////////////////////////UPDATING OUTPUT VARIABLES/////////////////////////////////////////        
    GList* temp_list2;
    int num_points;
    num_ped = new_ped + num_ped;
    (tc->peds)->num_ped = num_ped;
  
    for (k = 0; k < new_ped; k++)      //only add new pedestrian listing to ped_clust2
    {
        pedestrian_t *ped;
        ped = (pedestrian_t *) malloc(sizeof(pedestrian_t));
        ped->clust_card = 0;
	ped->deleted = 0;
        ped->cluster_list = NULL;
        (tc->peds)->ped_list = g_list_append((tc->peds)->ped_list, ped);
    }

    for (k = tr->start_peds_index; k < num_ped; k++)       //assign pedestrian cluster centers to each pedestrian
    {
        if ((tc->peds)->updated[k] == 1) //updated can never be 1 if deleted = 1
        {
            pedestrian_t *ped;
            ped = g_list_nth_data((tc->peds)->ped_list, k);
            clustering_delete_cluster_list(ped->cluster_list);
            ped->clust_card = pt_indx_ct[k];
            ped->cluster_list = NULL;

            for (j = 0; j < pt_indx_ct[k]; j++) // Number of clusters for each ped
            {                
                cluster_t *c2;
                c2 = (cluster_t *) malloc(sizeof(cluster_t));
                c2->mean_X = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->mean_X;
                c2->mean_Y = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->mean_Y;
                c2->dim_X = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->dim_X;
                c2->dim_Y = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->dim_Y;
                c2->num_pts = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->num_pts;
                c2->points_list=NULL;
                temp_list2 = ((cluster_t *) g_list_nth_data(curr, clust_group[k][j]))->points_list;
                num_points=0;

                while (temp_list2 != NULL)
                {
                    if (temp_list2->data != NULL)
                    {
                        point_t * p2;
                        p2 = (point_t*) malloc(sizeof(point_t));
                        p2->X=((point_t*)temp_list2->data)->X;
                        p2->Y=((point_t*)temp_list2->data)->Y;
                        c2->points_list= g_list_append(c2->points_list,p2);
                        num_points=num_points+1;
                    }
                    temp_list2 = g_list_next(temp_list2);		        
                }

                c2->num_pts=num_points;                  
                ((pedestrian_t *)g_list_nth_data((tc->peds)->ped_list, k))->cluster_list = g_list_append(((pedestrian_t *)g_list_nth_data((tc->peds)->ped_list, k))->cluster_list, c2);
                //printf("[temp_corr_calc.c] pedestrian:%d->cluster:%d->mean_X: %f\n",k,j,c2->mean_X);
            }           
        }
    }
   
    update *elt3;
    for (i = 0; i < ctr_alive;i++)   
    {        
	list* newrow2;
	newrow2 = (list*)g_list_nth_data(pref_table,i);
	
	while (newrow2->row != NULL)
    	{
        	elt3 = (newrow2->row)->data;
		free(elt3);
		newrow2->row = (newrow2->row)->next;
	}
	g_list_free(newrow2->row);
	free(newrow2);
    }
    g_list_free(pref_table);
    
    //printf("[temp_corr_calc.c] New peds detected on this scene: %d,\n                   Peds currently updated on scene: %d,\n                   Total peds so far: %d\n",new_ped,ctr,num_ped);
    free(close_pair[0]);
    free(close_pair);
    free(latelist);
    free(clustCentt1);
    //printf("[temp_corr_calc.c] All done\n");
    //ftime(&t2);
    //printf("\nTime to complete temp_corr_calc.c = %ld sec %ld ms\n",t2.time-t1.time,t2.millitm-t1.millitm);
    ////////////////////////////////////////OUTPUT UPDATE COMPLETE/////////////////////////////////////////////////
}

ped_set_t *temp_corr_calc_peds_get(temp_corr_calc_t * tc)
{
    assert(tc != NULL);

    return tc->peds;
}

void _temp_corr_print_cluster_list(temp_corr_calc_t * tc)
{
    assert(tc != NULL);
    assert(tc->peds != NULL);
    GList *n;
    pedestrian_t *ped;

    if (tc->peds->ped_list == NULL)
    {
        printf("[temp_corr_calc.c] Empty Cluster List\n");
    }
    else
    {
        printf("List = ");
        n = tc->peds->ped_list;
        while (n != NULL)
        {
            ped = (pedestrian_t *) (n->data);

            //printf("%f %f\t", ped->pos_X, ped->pos_Y);

            n = n->next;
        }
        printf("\n");

    }
}
