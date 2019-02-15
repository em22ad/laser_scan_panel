/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: clustering.c                     Date: 9 May. 2008 10:00 AM   **
** Author: Dylan A. Shell/Muhammad Emad                                 **
** Notes: Corrected clustering issues                                   **
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "clustering.h"
#include "svd2.c"
#include "tracking_common.h"
#include "utils.h"

// Forward decls of private functions.
void _clustering_init(clustering_t * cl);
void _clustering_shutdown(clustering_t * cl);
cluster_set_t *_clustering_get_current_cluster_set(clustering_t * cl);
void _clustering_toggle_current_cluster_set(clustering_t * cl);

clustering_t *clustering_create()
{
    clustering_t *ret;

    ret = (clustering_t *) malloc(sizeof(clustering_t));

    ret->cluster_set[0] = (cluster_set_t *) malloc(sizeof(cluster_set_t));
    ret->cluster_set[0]->cluster_list = NULL;
    ret->cluster_set[0]->numClust = 0;
    ret->cluster_set[0]->populated = 0;

    ret->cluster_set[1] = (cluster_set_t *) malloc(sizeof(cluster_set_t));
    ret->cluster_set[1]->cluster_list = NULL;
    ret->cluster_set[1]->numClust = 0;
    ret->cluster_set[1]->populated = 0;

    ret->current_cluster_set = 0;

    _clustering_init(ret);

    return ret;
}

void clustering_destroy(clustering_t * cl)
{
    assert(cl != NULL);
    _clustering_shutdown(cl);


    if ((cl->cluster_set[0])->cluster_list != NULL)     // if there exists a cluster there must be points in it, logically speaking
    {
        clustering_delete_cluster_list((cl->cluster_set[0])->cluster_list);
    }

    if ((cl->cluster_set[1])->cluster_list != NULL)     // if there exists a cluster there must be points in it, logically speaking
    {
       clustering_delete_cluster_list((cl->cluster_set[1])->cluster_list);
    }

    if ((cl->cluster_set[0]) != NULL)
        free(cl->cluster_set[0]);
    if ((cl->cluster_set[1]) != NULL)
        free(cl->cluster_set[1]);

    free(cl);
}


void _clustering_init(clustering_t * cl)
{
    assert(cl != NULL);
    printf("[Clustering] Init\n");
}

void _clustering_shutdown(clustering_t * cl)
{
    assert(cl != NULL);
    printf("[Clustering] Shutdown\n");
}

void pca(int k, cluster_set_t * output)
{
    //GList* y;                   //type point_t
    int j, error;
    double den;
    double e1, e2, sumx = 0.0, sumy = 0.0, means[2], c;
    long double sum1 = 0, sum2 = 0;

    static float **V1;
    static float **D;
    float *S1 = NULL;

    float *v1 = NULL;
    float *v2 = NULL;
    float *w1 = NULL;
    float *evals = NULL;
    float *w2 = NULL;
    int list_len;    
    cluster_t *cluster_ptr;
    cluster_ptr = (cluster_t *) g_list_nth_data(output->cluster_list, k);
    GList *pt_list_ptr2 = NULL;
    GList *temp=NULL;
    point_t * temp_point;

    
    pt_list_ptr2 = (GList *) ((cluster_t *) g_list_nth_data(output->cluster_list, k))->points_list;
    list_len = g_list_length(pt_list_ptr2);
    D = (float **)malloc(list_len*sizeof(float*));
    D[0] = (float *)malloc(list_len * 2*sizeof(float));
    V1 = (float **)malloc(list_len*sizeof(float *));
    V1[0] = (float *)malloc(list_len * 2*sizeof(float));
    S1 = (float *)calloc(2, sizeof(float));

    for (j=1;j<list_len;j++)
    {
        D[j]=D[0] + 2*j;
        V1[j] =V1[0] + 2*j;
    }

    if (list_len > 0)
    {
	temp=g_list_first(pt_list_ptr2);
	for (j = 0; j < list_len; j++)
        {
            temp_point=(point_t*)(temp->data);
	    sumx = sumx + temp_point->X;
            sumy = sumy + temp_point->Y;
	    temp = g_list_next(temp);
        }
        means[0] = sumx / list_len;
        means[1] = sumy / list_len;
    }
    else
    {
        cluster_ptr->dim_X = 0.0;
        cluster_ptr->dim_Y = 0.0;
        //printf("\n[clustering.c] Calculating PCA for zero points, please check\n");	
        return;
    }

    evals = (float *)calloc(list_len, sizeof(float));
    v1 = (float *)calloc(list_len, sizeof(float));
    v2 = (float *)calloc(list_len, sizeof(float));
    w1 = (float *)calloc(list_len, sizeof(float));
    w2 = (float *)calloc(list_len, sizeof(float));

    for (j = 0; j < list_len; j++)
    {
        D[j][0] = ((point_t *) g_list_nth_data(pt_list_ptr2, j))->X - means[0];
        D[j][1] = ((point_t *) g_list_nth_data(pt_list_ptr2, j))->Y - means[1];
    }

    if (list_len > 1)
    {
        error = dsvd(D,list_len,2, S1, V1);    //D gets overwritten by U
        //D gets over written by U
        //S1 has diagonal values of singular matrix
        //V1 is orthogonal transformation matrix
        if (error == 0)
        {
            cluster_ptr->dim_X = 0.0;
            cluster_ptr->dim_Y = 0.0;		
            return;
        }
        den = (float)(list_len - 1);

        if (den <= 0)
        {
            den = 1.0;  
        }

        for (j = 0; j < list_len; j++)
        {
            evals[j] = (SQR( (1.0 / den) * (S1[j])) );
        }
 
        if (evals[0] > evals[1])
        {
            e1 = evals[0];
            for (j = 0; j < list_len; j++)
            {
                v1[j] = V1[j][0];
                v2[j] = V1[j][1];

		if ((v1[j] > 3.0) || (v1[j] < -3.0))
		{
			v1[j]=0.0;
		}

		if ((v2[j] > 3.0) || (v2[j] < -3.0))
		{
			v2[j]=0.0;
		}
            }
            e2 = evals[1];
        }
        else
        {
            e1 = evals[1];
            for (j = 0; j < list_len; j++)
            {
                v1[j] = V1[j][1];
                v2[j] = V1[j][0];

		if ((v1[j] > 3.0) || (v1[j] < -3.0))
		{
			v1[j]=0.0;
		}

		if ((v2[j] > 3.0) || (v2[j] < -3.0))
		{
			v2[j]=0.0;
		}
             }
            e2 = evals[0];
        }


        c = 1.665;                  // 75'th percentile

        for (j = 0; j < list_len; j++)
        {
            w1[j] = c * sqrt(e1) * v1[j];
            w2[j] = c * sqrt(e2) * v2[j];
            sum1 = sum1 + SQR(w1[j]);
            sum2 = sum2 + SQR(w2[j]);
        }

    }
    else
    {
        /*e1 = 0;
        for (j = 0; j < g_list_length(pt_list_ptr2); j++)
        {
            v1[j] = V1[j][0];
            v2[j] = V1[j][1];
        }
        e2 = 0;*/

        // If we have so few numbers, we return  0, 0
   	cluster_ptr->dim_X = 0.0;
    	cluster_ptr->dim_Y = 0.0;
	return;
    }

    if ( isnan(sqrt(sum1)) )
    {
	cluster_ptr->dim_X = 0.0;		
    }else
    {
	cluster_ptr->dim_X = sqrt(sum1);
    }

    if ( isnan(sqrt(sum2)) )
    {
	cluster_ptr->dim_Y = 0.0;		
    }else
    {
    	cluster_ptr->dim_Y = sqrt(sum2);
    }

    //printf("[clustering.c]dim_X= %f, dim_Y= %f.\n",cluster_ptr->dim_X,cluster_ptr->dim_Y );
    //printf("[clustering.c] after dimension assignment in PCA\n");
    free(D[0]);
    free(D);
    free(V1[0]);
    free(V1);
    free(S1);
    free(evals);
    free(v1);
    free(v2);
    free(w1);
    free(w2);

}

void clustering_cluster_points(clustering_t * cl, double *X, double *Y, int N)
{
    cluster_set_t *output;
    int i, j;
    double bandwidth = 0.22;//0.8 //bigger the value larger the clusters
    double bandSq = SQR(bandwidth);
    double numInitPts = N;
    int *initPtInds;
    int *Visited;               // 0-1 list for visited status for each cluster
    int **clusterVotes;
    double stopThresh = 0.001 * bandwidth;
    int mergeWith, tempInd, stInd, ctr, cN = 0, numClust = 0, indx = 0; //ctr for inInds
    int *thisClusterVotes;
    int *qualified_cluster;
    double *sqDistToAll;
    int *inInds;
    double myMean[2], matrix[2], myOldMean[2]; 
    double sumx = 0.0, sumy = 0.0, distToOther = 0.0;
    double sqd;
    int rec;
    int maxval;
    int indrec = 0;
    int *data2cluster = NULL;

    assert(cl != NULL);
    assert(X != NULL);
    assert(Y != NULL);

    if (N == 0) return;

    _clustering_toggle_current_cluster_set(cl);
    output = _clustering_get_current_cluster_set(cl);

    output->populated = 1;
    if (output->cluster_list != NULL)
    { // Empty a non-empty cluster_list
        clustering_delete_cluster_list(output->cluster_list);
        output->cluster_list = NULL;
    }
    output->numClust = 0;

    clusterVotes = malloc(N * sizeof(int *));
    clusterVotes[0] = malloc((N * MAX_CLUSTERS_VAL) * sizeof(int));

    for (i = 1; i < N; i++)     
        clusterVotes[i] = clusterVotes[0] + i * MAX_CLUSTERS_VAL;

    for (i = 0; i < N; i++)    
    {
        for (j = 0; j < MAX_CLUSTERS_VAL; j++)
        {
            clusterVotes[i][j] = 0;
        }
    }

    Visited = (int *)calloc(N, sizeof(int));
    initPtInds = (int *)calloc(N, sizeof(int));
    qualified_cluster = (int *)calloc(N, sizeof(int));
    //printf("[clustering.c] Initialization finished.\n");

    thisClusterVotes = (int *)calloc(N, sizeof(int));
    sqDistToAll = (double *)calloc(N, sizeof(double));
    inInds = (int *)calloc(N, sizeof(int));

    for (i = 0; i < N; i++)
        thisClusterVotes[i] = sqDistToAll[i] = inInds[i] = 0;

    while (numInitPts != -1)
    {
        tempInd = ceil((numInitPts - 0.000001) * ((rand() % 100) / 100));       //pick a random seed point
        stInd = initPtInds[tempInd];    //use this point as start of mean       
        myMean[0] = X[stInd];
        myMean[1] = Y[stInd];   //intilize mean to this points location

        for (i = 0; i < N; i++)
        {
            thisClusterVotes[i] = 0;
        }

        while (1)               //loop until convergence
        {
            sumx = 0.0;
            sumy = 0.0;

            for (i = 0; i < N; i++)
            {
                inInds[i] = -1;
            }

            for (i = 0; i < N; i++)
            {
                matrix[0] = myMean[0] - (X[i]);
                matrix[1] = myMean[1] - (Y[i]);
                sqd = (SQR(matrix[0]) + SQR(matrix[1]));
                sqDistToAll[i] = sqd;
            }

            ctr = 0;
            //inInds must be cleared till this point                

            for (i = 0; i < N; i++)
            {
                if (sqDistToAll[i] < bandSq)
                {
                    inInds[ctr] = i;    //points within bandWidth
                    ctr++;
                    thisClusterVotes[i] = thisClusterVotes[i] + 1;      //add a vote for all the in points belonging to this cluster
                    sumx = sumx + X[i];
                    sumy = sumy + Y[i];
                }
            }

            myOldMean[0] = myMean[0];
            myOldMean[1] = myMean[1];
            myMean[0] = sumx / ctr;
            myMean[1] = sumy / ctr;

            for (i = 0; i < ctr; i++)
            {
                Visited[inInds[i]] = 1;
            }

            if (N < 1)
            {
                break;
            }     
            //printf("[clustering.c] Mean is converging\n");

            if (sqrt(SQR(myMean[0] - myOldMean[0]) + SQR(myMean[1] - myOldMean[1])) < stopThresh)
            {
                //check for merge posibilities                                          
                mergeWith = -1;
                for (cN = 0; cN < numClust; cN++)
                {
                    distToOther = sqrt(SQR(myMean[0] - ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_X) + SQR(myMean[1] - ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_Y));  //distance from possible new clust max to old clust max    
                    if (distToOther < (bandwidth / 2))  //if its within bandwidth/2 merge new and old
                    {
                        mergeWith = cN;
                        break;
                    }
                }

                if (mergeWith > -1)     // something to merge
                {
                    ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_X = 0.5 * (myMean[0] + ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_X);      //record the max as the mean of the two merged (I know biased twoards new ones)
                    ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_Y = 0.5 * (myMean[1] + ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->mean_Y);      //record the max as the mean of the two merged (I know biased twoards new ones)
                    //printf("[clustering.c] In mergewith\n");
                    for (indx = 0; indx < N; indx++)
                    {
                        clusterVotes[indx][mergeWith] = clusterVotes[indx][mergeWith] + thisClusterVotes[indx]; //add these votes to the merged cluster
                    }
                }
                else            //its a new cluster             
                {
                    numClust = numClust + 1;    //increment clusters
                    cluster_t *c2;
                    /*VAL*/                    c2 = (cluster_t *) malloc(sizeof(cluster_t));
                    c2->mean_X = myMean[0];
                    c2->mean_Y = myMean[1];
                    c2->num_pts = 0;
                    c2->points_list = NULL;
                    //output->numClust = output->numClust + 1;
                    //printf("[clustering.c] b4 append in cluster_list\n");
                    output->cluster_list = g_list_append(output->cluster_list, c2);     //pushed at numClust-1 index at centers         
                    //printf("[clustering.c] after append in cluster_list\n");
                    for (indx = 0; indx < N; indx++)
                    {
                        clusterVotes[indx][numClust - 1] = thisClusterVotes[indx];      //add these votes to the merged cluster                                                       
                    }
                }
                break;
            }
        }

        for (i = 0; i < N; i++)
        {
            initPtInds[i] = 0;
        }
        rec = -1;
        ctr = 0;

        for (indx = 0; indx < N; indx++)
        {
            if (Visited[indx] == 0)     //we can initialize with any of the points not yet visited
            {
                initPtInds[ctr] = indx;
                ctr++;
                rec++;
            }
        }
        numInitPts = rec;       //number of active points in set
    }
    //printf("[clustering.c] Voting finished\n");
    data2cluster = (int *)calloc(N, sizeof(int));

    for (i = 0; i < N; i++)
    {
        maxval = -1000;
        for (indx = 0; indx < numClust; indx++) //int(clusterVotes.size());indx++)
        {
            if (maxval < clusterVotes[i][indx])
            {
                maxval = clusterVotes[i][indx];
                indrec = indx;
            }
        }
        data2cluster[i] = indrec;       //a point belongs to the cluster with the most votes
    }

    /////////////////////*** If they want the cluster2data cell find it for them ***/////////////////////////////////////
    //printf("[clustering.c] Number of Clusters: %d \n", output->numClust);
    //printf("[clustering.c] Before points allocation\n");
    rec=0;
    for (cN = 0; cN < numClust; cN++)
    {
        ctr=0;        
        for (i = 0; i < N; i++)
        {
            if (data2cluster[i] == cN)
            {
                ctr=ctr+1;
                if (ctr > 1)
                {		
                    qualified_cluster[cN]=1;
                    rec=rec+1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < N; i++)
    {
        for (cN = 0; cN < numClust; cN++)
        {
            if ( (data2cluster[i] == cN) && (qualified_cluster[cN] == 1) )
            {              
                point_t *p1;
                p1 = (point_t *) malloc(sizeof(point_t));
                p1->X = X[i];
                p1->Y = Y[i];
                //points_list_ptr=((cluster_t *)g_list_nth_data(output->cluster_list,cN))->points_list;

                ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->points_list =
                    g_list_append(((cluster_t *) g_list_nth_data(output->cluster_list, cN))->points_list, p1);

                //printf("[clustering.c] Total points in cluster 0: %d \n",g_list_length(((cluster_t *)g_list_nth_data(output->cluster_list,cN))->points_list) );
                ((cluster_t *) g_list_nth_data(output->cluster_list, cN))->num_pts = ( ((cluster_t *)g_list_nth_data(output->cluster_list, cN))->num_pts) + 1;
            }
        }
    }
    output->numClust =rec;    
    //printf("[clustering.c] Before single point cluster deletion\n");
    GList *temp_list = g_list_first(output->cluster_list);
    GList *next = temp_list;

    while (temp_list != NULL)
    {
        next = temp_list->next;	
        if ( ((cluster_t*)(temp_list->data))->num_pts == 0)
        {	
            output->cluster_list = g_list_remove_link(output->cluster_list, temp_list);	
            free(temp_list->data);
        }
        temp_list = next;
    }    
    //////////////////////////////////////////////////START PCA SECTION////////////////////////////////////////////////
    //printf("[clustering.c] All finished before pca\n");
    for (cN = 0; cN < output->numClust; cN++)
    {
        pca(cN, output);
    }
    //printf("[clustering.c] PCA done\n");
    
    free(clusterVotes[0]);
    free(clusterVotes);
    free(Visited);
    free(initPtInds);
    free(thisClusterVotes);
    free(sqDistToAll);
    free(inInds);
    free(data2cluster);
    free(qualified_cluster);
    //printf("[clustering.c] Finished freeing. End of clustering\n");
}

cluster_set_t *_clustering_get_current_cluster_set(clustering_t * cl)
{
    assert(cl != NULL);
    return cl->cluster_set[cl->current_cluster_set];
}

void _clustering_toggle_current_cluster_set(clustering_t * cl)
{
    assert(cl != NULL);
    cl->current_cluster_set = (cl->current_cluster_set + 1) % 2;
    // printf("[clustering.c] writing cluster set: %d\n",cl->current_cluster_set);
}

cluster_set_t *clustering_clusters_old_get(clustering_t * cl)
{
    assert(cl != NULL);
    return cl->cluster_set[(cl->current_cluster_set + 1) % 2];
}

cluster_set_t *clustering_clusters_new_get(clustering_t * cl)
{
    assert(cl != NULL);
    return _clustering_get_current_cluster_set(cl);
}

int clustering_sets_populated(clustering_t *cl)
{
    assert(cl != NULL);
    return ((cl->cluster_set[0]->populated) && (cl->cluster_set[1]->populated));
}


void clustering_print_debug_set(cluster_set_t *out)
{
    GList *c;
    assert(out != NULL);

    printf("(%p)->numClust = %d\n", out, out->numClust);
    printf("(%p)->populated = %d\n", out, out->populated);
    printf("(%p)->cluster_list= %p\n", out, out->cluster_list);
    printf("(%p)->cluster_list has length %d\n", out, g_list_length(out->cluster_list));
    c = out->cluster_list;
    while (c != NULL)
    {
        printf("\t->data = %p\n", c->data);

        if (c->data != NULL)
        {
            clustering_print_debug_cluster(c->data);
        }

        c = c->next;
    }
}

void clustering_print_debug_cluster(cluster_t *cl)
{
    GList *p;
    assert(cl);

    printf("\t(%p)->mean_X = %f\n", cl, cl->mean_X);
    printf("\t(%p)->mean_Y = %f\n", cl, cl->mean_Y);
    printf("\t(%p)->dim_X = %f\n", cl, cl->dim_X);
    printf("\t(%p)->dim_Y = %f\n", cl, cl->dim_Y);
    printf("\t(%p)->num_pts = %d\n", cl, cl->num_pts);
    printf("\t(%p)->points_list= %p\n", cl, cl->points_list);
    printf("\t(%p)->points_list has length %d\n", cl, g_list_length(cl->points_list));
    p = cl->points_list;
    while (p != NULL)
    {
        printf("\t\t->data = %p\n", p->data);

        if (p->data != NULL)
        {
            clustering_print_debug_point(p->data);
        }

        p = p->next;
    }
}

void clustering_print_debug_point(point_t *pt)
{
    assert(pt != NULL);

    printf("(g_list_length(pt_list_ptr2) > 1)\t\t(%p)->X = %f\n", pt, pt->X);
    printf("\t\t(%p)->Y = %f\n", pt, pt->Y);
}

void clustering_delete_cluster_list(GList *cl)
{    
/*////////////////////////////////////////////////////////////////////////
	    GList *next;
	    GList *next2;
            GList *temp_list;
            GList *temp_list2;

            temp_list = cl;
            next = g_list_first(cl);

            while (next != NULL)
            {
		
               temp_list2 = ((cluster_t*)next)->points_list; 
               next2 = g_list_first(((cluster_t*)next)->points_list);

	       while (next2 != NULL)
	       {
               		temp_list2 = (GList *) g_list_delete_link(temp_list2, next2);
	                next2 = g_list_next(next2);
	       }
	       g_list_free( ((cluster_t*)next)->points_list );               
               temp_list = (GList *) g_list_delete_link(temp_list, next);
               next = g_list_next(next);
            }
            g_list_free( cl );                  
///////////////////////////////////////////////////////////////////////////////////////////////*/
    GList *w, *plist;
    cluster_t *cluster;
    point_t *point;

    if (cl == NULL) // empty list
        return;

    w = cl;
    while (w != NULL)
    {
        cluster = w->data;

        plist = cluster->points_list;
        while (plist != NULL)
        {
            point = plist->data;

            free(point);

            plist = plist->next;
        }

        g_list_free(cluster->points_list);
        free(cluster);

        w = w->next;
    }

    g_list_free(cl);
}


