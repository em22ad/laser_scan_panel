/*************************************************************************
**      Laser Scan Control Panel                                        **
** File: tracking.c                               Date: 08 Apr. 2008    **
** Author: Dylan A. Shell/Muhammad Emad                                 **
** Notes:                                                               **
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "tracking.h"
#include <math.h>
#include "utils.h"
//#include <sys/timeb.h>

// Forward decls.
void _tracking_init(tracking_t * tr);
void _tracking_shutdown(tracking_t * tr);
void _tracking_clean_up_people_list(tracking_t * tr);

tracking_t *tracking_create()
{
    int i;
    tracking_t *ret = NULL;
    ret = (tracking_t *) malloc(sizeof(tracking_t));
    // Set members
    ret->num_ppl = 0;
    ret->start_peds_index = 0; 

    for (i = 0; i < MAX_PED_VAL; i++)
    {
        ret->people[i].to_be_deleted = 0;
        ret->people[i].initialized = 0;
        ret->people[i].life=PED_LIFE;
        ret->people[i].updated = 0;
        ret->people[i].first_update = 0;
        ret->people[i].X = 0.0;
        ret->people[i].Y = 0.0;
        ret->people[i].est_X = 0.0;
        ret->people[i].est_Y = 0.0;
        // particles initialized in tracking_process(tracking_t * tr, ped_set_t * peds)
    }
    _tracking_init(ret);
    return ret;
}

void tracking_destroy(tracking_t * tr)
{
    assert(tr != NULL);
    _tracking_shutdown(tr);

    free(tr);
}

void tracking_get_peds(tracking_t * tr, double *X, double *Y, int *N,
                       int max_output_size)
{
    assert(tr != NULL);
    assert(X != NULL);
    assert(Y != NULL);
    assert(N != NULL);
    //printf("[Tracking] Need to service request to return pedestrian (given output arrays X and Y), to write a maximum of max_output_size, and the total number written to N.\n");
}

void _tracking_init(tracking_t * tr)
{
    assert(tr != NULL);
    printf("[Tracking] Init\n");
}

void _tracking_shutdown(tracking_t * tr)
{
    assert(tr != NULL);
    printf("[Tracking] Shutdown\n");
}

void tracking_process(tracking_t * tr, ped_set_t * peds)
{
    #define MAX_ALIVE_PEOPLE 300 /* Set this to something more appropriate */

    //struct timeb t1;
    //struct timeb t2;
    //ftime(&t1);   
    //printf("[tracking.c] tracking started.\n");
    assert(tr != NULL);
    assert(peds != NULL);

    int N = peds->num_ped;      // No. of pedestrians 0 <= N <= MAX_PED_VAL
    int M = NUM_PARTICLES;      // No. of particles...lets keep it 300 for now...later we can increase it to enhance 
    int ii,i = 0;                  // maximum clusters allowed per pedestrian
    int *pt_indx_ct;            // no of clusters associated to each pedestrian
    static double locw[MAX_ALIVE_PEOPLE][NUM_PARTICLES];     // local variable for particle weights of dimensions (N,M);             
    GList *list;
    int j, ctr;
    double sumxl,sumyl,sumx, sumy, sumw, x_comp, y_comp;
    double time_int = 0.025;     // time between t and t-1 from observations by looking at data
    tr->num_ppl = N;
    int alive_peds;
    static int pt_indx_ct_static[MAX_PED_VAL+1];
    #define sqrt_6285 2.506990227344335

/*    static double last_time_exit = -1;
    static double time_outside = 0.0;
    static double time_pinit = 0.0;
    static double time_update = 0.0;
    static double time_sampling = 0.0;
    static double time_pred = 0.0;
    static double time_total = 0.0;
    static long time_write_counter = 0;*/

    //double call_time = get_current_time();
    GList *temp_list = NULL;
    GList *cluster_list = NULL;
    cluster_t *temp_clust;

    /////////////////////////////////////////////////////ALLOCATE MEM///////////////////////////////////////////////////
    //pt_indx_ct = (int *)calloc(N, sizeof(int));
    // We use a static version now:

    pt_indx_ct = pt_indx_ct_static;
    /////////////////////////////////////////////PARTICLE INITIALIZATION/////////////////////////////////////////////////
    
    alive_peds = 0;
    for (i = tr->start_peds_index; i < N; i++)     
    {
        if ((peds->updated[i] == 1) || (tr->people[i].life > 0))
        {
            if (alive_peds == 0)
            { // up till now there have been only dead pedestrians
                tr->start_peds_index = MAX(0,i-1);
            }
            alive_peds++;
        }
    }
    
    //printf("[tracking.c] start_peds_index = %d\n",tr->start_peds_index);
    //printf("N = %d; alive peds = %d < %d\n", N, alive_peds, MAX_ALIVE_PEOPLE);
    assert(alive_peds < MAX_ALIVE_PEOPLE); //Ensure we have enough allocated.

    ii=0;
    for (i = tr->start_peds_index; i < N; i++)     //for each pedestrian
    {
        pt_indx_ct[i] = ((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->clust_card;
        tr->people[i].updated=0;

        if (peds->updated[i] == 1)
        {
            tr->people[i].updated = 1;
            tr->people[i].life=PED_LIFE;		

            if (tr->people[i].initialized == 0)     //do not initialize at every run
            {
                sumx = 0.000;
                sumy = 0.000;        
                list = (GList *) ((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->cluster_list;

                cluster_list = g_list_first(list);
                for (ctr = 0; ctr < pt_indx_ct[i]; ctr++)
                {
                    //printf("[tracking.c] total clusters %d.\n",g_list_length(list));              
                    temp_clust = (cluster_t *)(cluster_list->data);
                    x_comp = (double)(temp_clust->mean_X);
                    y_comp = (double)(temp_clust->mean_Y);
                    
                    sumx = sumx + x_comp;
                    sumy = sumy + y_comp;
                    
                    cluster_list = g_list_next(cluster_list);
                }

                tr->people[i].X = sumx / pt_indx_ct[i];
                tr->people[i].Y = sumy / pt_indx_ct[i];

                //printf("\n\n\n[tracking.c] Initialized on new pedestrian detection.\n\n\n");
                tr->people[i].initialized = 1;	

                for (j = 0; j < M; j++)     // for each particle
                {
                    tr->people[i].particles[j].phase = uniform_rand() * 360.000;
                    tr->people[i].particles[j].period = 1.0 ; // 0.5 is half the time taken by a person to take 2 steps i.e. 1 sec
                    tr->people[i].particles[j].dir = uniform_rand() * 360.000;
                    //tr->people[i].particles[j].stride = 0.6; //maximum distace between two feet
                    tr->people[i].particles[j].stride = uniform_rand(); //maximum distace between two feet
                    tr->people[i].particles[j].weight=(1.0/M);//weight equally distirbuted among M particles

                    tr->people[i].particles[j].x[0] = tr->people[i].X + INIT_PARTICLE_DEV * uniform_rand();       //x-axis of right foot, o correspondos to right foot
                    tr->people[i].particles[j].y[0] = tr->people[i].Y + INIT_PARTICLE_DEV * uniform_rand();       //y-axis of right foot

                    tr->people[i].particles[j].x[1] = tr->people[i].particles[j].x[0] + INIT_PARTICLE_DEV * uniform_rand();       //x-axis of left foot , 0.05 + 0.05= 0.1 will be the max distance between right and left foot particles
                    tr->people[i].particles[j].y[1] = tr->people[i].particles[j].y[0] + INIT_PARTICLE_DEV * uniform_rand();       //y-axis of right foot 

                }
            }//end if
        }
        else
        {
            if (tr->people[i].life > 0)		
            {
                tr->people[i].life=tr->people[i].life-1;

                for (j = 0; j < M; j++)     // for each particle
                {
                    locw[ii][j]=tr->people[i].particles[j].weight;
                }
            }
        }//end updated if*/

        if (tr->people[i].life > 0)
        {
            ii++;
        }
    }// end pedestrian for
    //printf("[tracking.c] finished particle initialization.\n");   
    //////////////////////////////////////////////////////PARTICLE INITIALIZATON COMPLETE///////////////////////////////////////////////////////////////////////////////////        
    
    //double call_time2 = get_current_time();

    //////////////////////////////////////////////////////UPDATE STEP STARTS HERE//////////////////////////////////////////////////////////////////////
    //double covL = 0.1, covR = 0.1;      // bigger number means wider spectrum of particles will be selected i.e. more noise will be incorprated // effects greatly motion of particles having no updates
    double covS = 0.2, h = 0.2;		 //0.1; Keep h low for consistent results % stride tolerance -(x_dir(i,j)-x_phase(i,j));    % finalize what it is, may be its time difference
    double sumprob, sumL, sumR, cjR, cjL;
    static double prob[MAX_ALIVE_PEOPLE][NUM_PARTICLES];     
    double d0, r0, pB;
    int c, v;
    GList *points_list = NULL;
    cluster_list = NULL;

    point_t *temp_pt;
    ////////////ALLOCATE MEM////////////////////////////
    i = tr->start_peds_index;
    //tracking_display_peds(tr,peds);

    ii = 0;
    while (i < N)               // loop will run once for each dectected pedestrian i is index for projected pedestrians
    {
        sumprob = 0.000;
        temp_list = (GList *) ((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->cluster_list;
        //printf("[tracking.c] ped-> %d.\n",i);                         
        if (peds->updated[i] == 1)
        {
            //printf("[tracking.c] start for loop: %d.\n",i);       
            for (j = 0; j < M; j++)
            {
                sumL = 0.000;
            	sumR = 0.000;
                //printf("[tracking.c] Inside ped pos update\n",j);                              
                cluster_list = g_list_first(temp_list);
                for (c = 0; c < pt_indx_ct[i]; c++)
                {
                    if (cluster_list == NULL)
                    {
                        printf("The cluster list has fewer than the pt_indx_ct[.] entries expected.\n");
                        break;
                    }

                    temp_clust = (cluster_t *)(cluster_list->data);
                    //printf("[tracking.c] cluster must have pts > 0 : %d\n",temp_clust->num_pts);
                    
                    v = 0;
                    for (points_list =  g_list_first(temp_clust->points_list); points_list; points_list = g_list_next(points_list))
                    {
                         
                        if (v >= temp_clust->num_pts)
                        {
                            printf("The points in the points list has more than temp_clust->num_pts entries.\n");
                            break;
                        }

                        temp_pt = (point_t *)(points_list->data);

                        //if ((i == 1) && (j == 0) && (c == 0))
                        //printf("[tracking.c] ped->%d, cluster->%d, point->%d, value_X->%f\n",i,c,v,temp_pt->X);                                               
                        sumL =
                            sumL +
                            exp(- (SQR(tr->people[i].particles[j].x[1] - temp_pt->X) + SQR(tr->people[i].particles[j].y[1] - temp_pt->Y)) / (2.000 * SQR(covS)) );
                        sumR =
                            sumR +
                            exp(- (SQR(tr->people[i].particles[j].x[0] - temp_pt->X) + SQR(tr->people[i].particles[j].y[0] - temp_pt->Y)) / (2.000 * SQR(covS)) );
                        v++;
                        
                    }

                    cluster_list = g_list_next(cluster_list);
                }

                d0 = (tr->people[i].particles[j].x[0] - tr->people[i].particles[j].x[1]) * cos(deg_to_rad(tr->people[i].particles[j].dir)) +
                    (tr->people[i].particles[j].y[0] - tr->people[i].particles[j].y[1]) * sin(deg_to_rad(tr->people[i].particles[j].dir));
                r0 = -tr->people[i].particles[j].stride * cos(deg_to_rad(tr->people[i].particles[j].phase));
                pB = (1.000 / (sqrt_6285 * h)) * exp(-(SQR(d0 - r0)) / (2.0 * SQR(h)));
                cjR = (1.000 / (sqrt_6285 * covS)) * sumR;
                cjL = (1.000 / (sqrt_6285 * covS)) * sumL;
                prob[ii][j] = cjR * cjL * pB;
                sumprob = sumprob + (prob[ii][j]);
            }// end for loop for M
        } //end if  

        if (peds->updated[i] == 1) //|| (tr->people[i].life > 0))
        {
            for (j = 0; j < M; j++) // for each particle
            {
                if (sumprob > 0.0)
                {
                    locw[ii][j] = prob[ii][j] / sumprob;      //normalization step
                }
                //printf("[tracking.c] Probability of %d pedestrian for its %d particle: %f\n",i,j,locw[i][j]);       		
                tr->people[i].particles[j].weight = locw[ii][j];
            }
        }

        if (tr->people[i].life > 0)
        	ii++;

        i = i + 1;
    }                           // end of dectected pedestrians while loop
    //////////////////////////////////////////////////End of Update step///////////////////////////////////////////////////
    
    //double call_time3 = get_current_time();

    //////////////////////////////////////////////Sampling Step starts here////////////////////////////////////////////////
    double r;
    static int locs[MAX_ALIVE_PEOPLE][NUM_PARTICLES];        //array which holds sample index of all particles of dimension (N,M);// Random numbers for Max number of pedestrians anticipated to be detected

    ii = 0;
    for (i = tr->start_peds_index; i < N; i++)     // for each pedestrian
    {
        if (peds->updated[i] == 1) //|| (tr->people[i].life > 0))
        {
            for (j = 0; j < M - 1; j++)     // for each particle
            {
                locw[ii][j+1] = locw[ii][j] + (locw[ii][j+1]);
            }
        }
        
        if (tr->people[i].life > 0)
        {
       		ii++;
        }
    }
    // Okay, now locw[i][....] has summed numbers up to locw[i][NUM_PARTICLES - 1] == 1.0 

    ii = 0;
    for (i = tr->start_peds_index; i < N; i++)     // for each pedestrian
    {
        if ( (peds->updated[i] == 1) || (tr->people[i].life > 0) )
        {
            for (j = 0; j < M; j++) // for each particle
            {
                /////////////////////////Random No. generation routine////////////////
                locs[ii][j] = 0;
                r = uniform_rand(); //select a random number that runs through out one iteration
                //printf("\n random: %f\n",r);
                //////////////////////////////////////////////////////////////////////
                while (1)
                {
                    if (!(locs[ii][j] < M))
                        break;
                    if (!(locw[ii][locs[ii][j]] < r))
                        break;
                    locs[ii][j] = (locs[ii][j]) + 1;
                }

                if (locs[ii][j] > M - 1)
                {
                    locs[ii][j] = M - 1;
                }

                if (locs[ii][j] < 0)
                {
                    locs[ii][j] = 0;
                }
            }
        }
        
        if ((peds->updated[i] == 1) || (tr->people[i].life > 0))
            ii++;
    }
    // locs[i] will have randomly sampled with a distribution based.
    
    //printf("[tracking.c] finished sampling step.\n");
    ////////////////////////////////////////////////////End sampling step/////////////////////////////////////////////
    
    //double call_time4 = get_current_time();

    ///////////////////////////////////////////////Prediction step starts here ///////////////////////////////////////
    double ROT[2][2];
    int tindex;
    double r1;           //moving foot motion range, less makes tracking concentrated, more spreads it
    double r2, md, rL, rR, one, two;  //still foor motion range, takes care of the mapping of the actual location of clusters as well
    double *x_stridet;
    double *x_periodt;
    double *x_dirt;
    double *x_phaset;
    double  noise_ph,noise_ph1;
    double noise_dir,noise_dir1;
    double noise_time,noise_time1;
    double noise_str,noise_str1;
    static double x_stridet_static[NUM_PARTICLES+1];
    static double x_periodt_static[NUM_PARTICLES+1];
    static double x_phaset_static[NUM_PARTICLES+1];
    static double x_dirt_static[NUM_PARTICLES+1];

    ////////////ALLOCATE MEM////////////////////////////
    // This is made to static arrays in order save the 
    // allocation time.
    //x_stridet = (double *)calloc(M, sizeof(double));
    //x_periodt = (double *)calloc(M, sizeof(double));
    //x_phaset = (double *)calloc(M, sizeof(double));
    //x_dirt = (double *)calloc(M, sizeof(double));
    
    x_stridet = x_stridet_static;
    x_periodt = x_periodt_static;
    x_phaset = x_phaset_static;
    x_dirt = x_dirt_static;

    ii = 0;
    for (i = tr->start_peds_index; i < N; i++)     //for each pedestrian, predict phase
    {

        if (tr->people[i].life > 0)
        {

            if (tr->people[i].life >= PED_LIFE-1)
            {
                r1 = 0.08;        //0.08; // rate of motion
                r2 = 0.0008;
                noise_ph1 = 6.0;     // degree change in phase during walk
                noise_dir1 = 30.0;     //15 degree noise in direction
                noise_time1 = 0.002;     // 1% deviation in the walking time period of person
                noise_str1 = 0.5;    //0.00125; //20% deviation in maximum distace between two feet//controls the distance we expect our pedestrian to travel while random
            }
            else
            {
                r1 = 0.05;         // spread of particles should increase considerably
                r2 = 0.0005;         // low value keeps the distance between feet less
                noise_ph1 = 50.0;//30.0;    //( (1/(tr->people[i].life+1)/PED_LIFE) );   //phase becomes un-predictable
                noise_dir1 = 180.0;    //( (1/(tr->people[i].life+1)/PED_LIFE) ); //direction becomes un-predictable
                noise_time1 = 0.02;    //(PED_LIFE/tr->people[i].life)/100; //walking time period increases a lot but has a limit//cranking up this term makes peds change feet far slower
                noise_str1 = 0.005;    //( (1/(tr->people[i].life+1)/PED_LIFE) ); //stride remains predictable    
            } 

            //printf("[tracking.c] Inside propagation\n");		
            for (j = 0; j < M; j++)
            {
                if (tr->people[i].life >= PED_LIFE)
                {
                    x_phaset[j] = tr->people[i].particles[j].phase;
                    x_periodt[j] = tr->people[i].particles[j].period;
                    x_stridet[j] = tr->people[i].particles[j].stride;
                    x_dirt[j] = tr->people[i].particles[j].dir;
                }else
                {
                    x_phaset[j] = tr->people[i].particles[j].phase;
                    x_periodt[j] = tr->people[i].particles[j].period;
                    x_stridet[j] = 0.005;
                    x_dirt[j] = tr->people[i].particles[j].dir;		
                }
            }

            for (j = 0; j < M; j++) // for each particle
            {
                tindex = locs[ii][j];

                noise_ph = noise_ph1 * (normal_rand());     // DOES NOT MATCH WIT MATLAB CONFIG usual noise that is encountered during phase update %%% increase makes consecutive readings spread in terms of angle (general direction grows more towards x-axis as compared to y)
                noise_dir = noise_dir1 * (normal_rand());   // usual noise that is encountered during direction update %%% increase makes readings overshoot both x=axis rangeand y-axis range
                noise_time = noise_time1 * (normal_rand());      // usual noise that is encountered during time period update %%% increase makes jumps too small % decrease makes trajectory perfect with less spread but over shoots x,y-range, CONTROLS x-range
                noise_str = noise_str1 * (normal_rand());       //increase to reach faster to the original values    // usual noise that is encountered during stride update, controls x, y range

                tr->people[i].particles[j].phase = x_phaset[tindex] + 2.000 * (90.000) * (time_int / tr->people[i].particles[tindex].period) + noise_ph;
                tr->people[i].particles[j].period = x_periodt[tindex] + noise_time;
                tr->people[i].particles[j].stride = x_stridet[tindex] + noise_str;
                tr->people[i].particles[j].dir = x_dirt[tindex] + noise_dir;//location probability gets unpredictable
                // above noise parameters need to be adjusted in order to induce
                // realistic motion in pedestrian behavior
            }

            for (j = 0; j < M; j++) // for each particle
            {
                tindex = locs[ii][j];
                md = tr->people[i].particles[tindex].stride *(fabs (cos(deg_to_rad(tr->people[i].particles[j].phase)) - cos(deg_to_rad(x_phaset[tindex]))));	// add distance to the moving foot

                if (fmod((tr->people[i].particles[j].phase),360.0) > 180.0)
                {
                    rL = r1*uniform_rand() + md;  // left foot moving 
                    rR = r2*uniform_rand();
                }
                else
                {
                    rR = r1*uniform_rand() + md;  // right foot moving 
                    rL = r2*uniform_rand();
                }
                one = cos(deg_to_rad(tr->people[i].particles[tindex].dir));
                two = -sin(deg_to_rad(tr->people[i].particles[tindex].dir));
                ROT[0][0] = one;
                ROT[0][1] = two;
                ROT[1][0] = -two;
                ROT[1][1] = one;
                tr->people[i].particles[j].x[1] = tr->people[i].particles[tindex].x[1] + ((ROT[0][0] * rL) + (ROT[0][1] * rL));
                tr->people[i].particles[j].y[1] = tr->people[i].particles[tindex].y[1] + ((ROT[1][0] * rL) + (ROT[1][1] * rL));
                tr->people[i].particles[j].x[0] = tr->people[i].particles[tindex].x[0] + ((ROT[0][0] * rR) + (ROT[0][1] * rR));
                tr->people[i].particles[j].y[0] = tr->people[i].particles[tindex].y[0] + ((ROT[1][0] * rR) + (ROT[1][1] * rR));
            }
        }//end updated if

        if (tr->people[i].life > 0)
        {
            ii++;
        }
    }

    //ii = 0;
    for (i = tr->start_peds_index; i < N; i++)
    {
        if ( tr->people[i].life > 0)
        {
            //printf("[tracking.c] Inside estimation update\n");
            sumx = 0.0;
            sumy = 0.0;
            sumxl = 0.0;
            sumyl = 0.0;
            sumw = 0.0;

            for (j = 0; j < M; j++)
            {
                //tindex = locs[ii][j];
                sumw += tr->people[i].particles[j].weight;
                sumx += tr->people[i].particles[j].weight*tr->people[i].particles[j].x[0];
                sumy += tr->people[i].particles[j].weight*tr->people[i].particles[j].y[0];
                sumxl += tr->people[i].particles[j].weight*tr->people[i].particles[j].x[1];
                sumyl += tr->people[i].particles[j].weight*tr->people[i].particles[j].y[1];
                //sumx += tr->people[i].particles[tindex].x[0];
                //sumy += tr->people[i].particles[tindex].y[0];
            }
            //tr->people[i].est_X = sumx / M;
            //tr->people[i].est_Y = sumy / M;
            tr->people[i].est_X = (sumx+sumxl) / (2.0*sumw);
            tr->people[i].est_Y = (sumy+sumyl) / (2.0*sumw);            
            //tr->people[i].est_X = tr->people[i].X;
            //tr->people[i].est_Y = tr->people[i].Y;

            //printf("[tracking.c] Diff in Extimated and Actual X: %f\n             Diff in Estimated and Actual Y: %f\n",tr->people[i].est_X-tr->people[i].X,tr->people[i].est_Y-tr->people[i].Y); 
        }

        //if (tr->people[i].life > 0)
        //    ii++;
    }                           //end xest yest calculation
    tr->num_ppl = N;
    ///////////////////////////////////////////////////End Prediction step///////////////////////////////////////////////// 
    
    //if (last_time_exit < 0)
    //    last_time_exit = call_time;

    //double call_time5 = get_current_time();

    /*time_pinit = time_pinit + (call_time2 - call_time);
    time_update = time_update + (call_time3 - call_time2);
    time_sampling = time_sampling + (call_time4 - call_time3);
    time_pred = time_pred + (call_time5 - call_time4);
    time_total = time_total + (call_time5 - call_time);
    time_outside = time_outside + (call_time - last_time_exit);*/

    /*if ((time_write_counter % 100) == 0)
    {
        printf("(SUM of %d) P_INIT=%fs\t", 100, time_pinit);
        printf("UPDATE=%fs\t", time_update);
        printf("SAMPLING=%fs\t", time_sampling);
        printf("PRED=%fs\t", time_pred);
        printf("TOTAL=%fs\t", time_total);
        printf("OUTSIDE=%fs\n", time_outside);

        time_pinit = 0.0;
        time_update = 0.0;
        time_sampling = 0.0;
        time_pred = 0.0;
        time_total = 0.0;
        time_outside = 0.0;

    }
    time_write_counter++;*/

    
    // All these frees are unnecessary since we use static arrays allocated once now.
    //free(x_stridet);
    //free(x_periodt);
    //free(x_dirt);
    //free(x_phaset);
    //free(pt_indx_ct);

    //last_time_exit = get_current_time();
}

void tracking_display_peds(tracking_t * tr, ped_set_t * peds)
{
 	int i=0,c,v;
	GList *temp_list = NULL;
    	cluster_t *temp_clust;
    	point_t *temp_pt;   

	while (i < peds->num_ped)               // loop will run once for each dectected pedestrian i is index for projected pedestrians
	{
		temp_list = (GList *) ((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->cluster_list;
                     
		if (peds->updated[i] == 1)
		{
			printf("\n[tracking.c] ped-> %d has been updated and has %d clusters.\n",i,((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->clust_card);    			
			for (c = 0; c < ((pedestrian_t *) g_list_nth_data(peds->ped_list, i))->clust_card; c++)
			{
				temp_clust = (cluster_t *) g_list_nth_data(temp_list, c);
				printf("[tracking.c] \tCluster %d has %d points\n",c,temp_clust->num_pts);
				for (v = 0; v < temp_clust->num_pts; v++)
				{
					temp_pt = (point_t *) g_list_nth_data(temp_clust->points_list, v);
					printf("[tracking.c] \t\tped->%d, cluster->%d, point->%d, value_X->%f, value_Y->%f\n",i,c,v,temp_pt->X,temp_pt->Y);                                               
			    	}
			}
		}
		i = i + 1;
	}// end major while
}

void _tracking_clean_up_people_list(tracking_t * tr)
{
    int i, j;
    int num_del;
    assert(tr != NULL);
    // Go through each of the people, and delete the ones
    // that have the to_be_deleted flag set.

    num_del = 0;
    i = j = 0;
    while (j < tr->num_ppl)
    {
        if (!tr->people[j].to_be_deleted)
        {
            if (i != j)
            {
                tr->people[i] = tr->people[j];
            }
            else
            {
                //No Op.
            }
            i++;
            j++;
        }
        else
        {
            num_del++;
            j++;
        }
    }

    tr->num_ppl = tr->num_ppl - num_del;
}
