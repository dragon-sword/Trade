
// 価各種単位は統一
// 電力価格・ペナルティ：円/wh
// 電力量：wh

#define _CRT_SECURE_NO_WARNINGS
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <tchar.h>
#pragma comment(lib,"odbc32.lib")
int ID,M_N,D,Load_cuvs,FMStep,Elecstep,BatStep,TimeStep,Scale,Byo,P_num,Max_b,Max_bph,Sta_d;
int *Agree_t;
double *Agree_ord_p,*Agree_p,*Agree_q;
double *EM_gzai1,*EM_gzai2,*EM_ux1,*EM_ux2,*EM_uy1,*EM_uy2,*EM_sigma2,*EM_sigmaxy1,*EM_sigmaxy2;
double **EM_p;
int *ykj;
int *Can_t,*Can_p,*Can_q;
int *Tra_t;
int **Trading;
int *Act_t;
int **Bids,Bidpat;
int ini,ykjMAX;
double M_dev1,M_dev2,T_dev,Batmax,UnitBat,Max_bwh,Unit_bid,Min_p,Max_p,Unit_p,Power;
double Start;
double *PVout,*Pe,*Bat_eff;
double ux[2],uy[2];
double ux_day1,ux_day2,ux_night1,ux_night2,uy_day1,uy_day2,uy_night1,uy_night2;
double sigma2,sigmaxy[2];
double gzai[2];
double **Lc_elecs;
double *D_cost;
char Name;
HENV   henv;
HDBC   hdbc;
HSTMT  hstmt;
int buffsize = 256;
TCHAR SQLbuff[256];
void connectmarket(),con(),init(),readfile(double*),make_bidpat(),make_loadpat(int,int*);
void init_prob2(double**,double****,int*);
void alarm();
void readfile1(char*,int,double**);
void readfile2(char *);
void readfile3(char*,int*,double*,double*,double*);
void readfile4(int*,HENV,HDBC,HSTMT);
void readfile5(char*,int,int,double***);
void result1(char*,int**,int*,int);
void result2(char*,int**,double**);
void result3(char*,int*,double*,double*,int);
void result4(int d,int*,double*,double*,int*,double*,double);
void result5(int d,double***);
void resulttest(int**);
void result8(char*,double*,double*,double*,double*,double*,double*,double*,double*,double*);
void bi(int,double,int,int,int*,int*,int***,HENV,HDBC,HSTMT);
void agree_order(int,int**,HENV,HDBC,HSTMT);
void get_keep1(int,double*,double*,double*,double,int*,double,double**,int*,HENV,HDBC,HSTMT);
void get_keep2(int,HENV,HDBC,HSTMT);
void cancel(int,int,int*,HENV,HDBC,HSTMT);
int agree_cancel(int,int,int*,HENV,HDBC,HSTMT);
void get_rec(int,double*,double*,double*,int*,double,double**,int*,HENV,HDBC,HSTMT);
void StochasticDynamicProgramming1(int,int,double***,double***,int***);
void calc_Q(int,int,int,int*,int,int,int,double*,double*);
void action(int,double*,double*,double*,double*,double,double);
void EMalgorithm(int,int,double*,double*,double*,double,double**,double***,double*,int*);
void dec_d_pat(int*,int*);
void what_time(double,int);
double d_sec(int);//その日の時刻(秒)
double dis(),t_dis();
int *m_iarr1(int);
int **m_iarr2(int,int);
int ***m_iarr3(int,int,int);
int ****m_iarr4(int,int,int,int);
double *m_darr1(int);
double **m_darr2(int,int);
double ***m_darr3(int,int,int);
double ****m_darr4(int,int,int,int);
void f_iarr2(int**,int,int);
void f_iarr2x(int**,int);
void f_darr2x(double**,int);
void f_darr2(double**,int,int);
int *ex_iarr1(int*,int);
int **ex_iarr2(int**,int);
struct bid *m_bid_arr1(int);
void MakeTable(HENV, HDBC,HSTMT),DropTable(HENV, HDBC,HSTMT);
bool Connect(TCHAR*,TCHAR*,TCHAR*,HENV*, HDBC*,HSTMT*);
void Close(HENV*, HDBC*,HSTMT*);
bool Execute(HENV*, HDBC*, HSTMT*,TCHAR*);
void Count(HENV, HDBC, HSTMT, int*, TCHAR*);
///////////////////////////////////////////////////////////////////////////////////////

void main(){
	int k,day,loadpat,bnum;
	int rank;
	int *branks;
	int **m, **step;
	int rec_n,can_n,tra_n,act_n;
	int ***a_table;
	double kan,bidtime,baff,bat,t_cost;
	double *shor,*cost,**lc_elec,*battery;
	double ***trans_m,***prob_m,***q_table;
	double ***prob_m2;
	
	//市場への接続//
	connectmarket();
	//定数の決定//
	con();
	//データの取得//
	readfile(&bat);
	//接続 ODBCデータソース名 ユーザー名 パスワード
	_stprintf_s(SQLbuff,buffsize,TEXT("SQLtoMarket%d"),M_N);
	if(!Connect(SQLbuff,TEXT("inoue"),TEXT("Inojun5811"),&henv,&hdbc,&hstmt)){
		printf("接続エラー\n");
	}
	//必要なテーブルの作成//
	MakeTable(&henv, &hdbc, &hstmt);

	k=0;
	day=0;
	rank=0;
	bidtime=0;
	baff=0;
	bnum=0;
	rec_n=0;
	can_n=0;
	tra_n=0;
	act_n=0;
	branks=NULL;

	shor=m_darr1(FMStep+1);
	battery=m_darr1(FMStep+1);
	cost=m_darr1(FMStep+1);
	lc_elec=m_darr2(D,FMStep+1);
	ykj=m_iarr1(TimeStep);
	m=m_iarr2(D,FMStep+1);
	step=m_iarr2(D,FMStep+1);
	a_table=m_iarr3(TimeStep,BatStep,P_num);
	q_table=m_darr3(TimeStep,BatStep,P_num);
	Trading=m_iarr2(1,P_num);
	trans_m=m_darr3(TimeStep,P_num,P_num);
	prob_m=m_darr3(TimeStep,P_num,P_num);
	prob_m2=m_darr3(TimeStep,P_num,P_num);
	EM_p=m_darr2(TimeStep,ykjMAX);
	init_prob2(EM_p,&(prob_m2),ykj);
	for(int i=0;i<P_num;i++)Trading[0][i]=0;
	Tra_t=m_iarr1(1);Tra_t[0]=0;
	Can_t=m_iarr1(1);Can_t[0]=0;
	Can_p=m_iarr1(1);Can_p[0]=0;
	Can_q=m_iarr1(1);Can_q[0]=0;
	Agree_t=m_iarr1(1);Agree_t[0]=0;
	Agree_p=m_darr1(1);Agree_p[0]=0;
	Agree_q=m_darr1(1);Agree_q[0]=0;
	Agree_ord_p=m_darr1(1);Agree_ord_p[0]=0;
	D_cost=m_darr1(D);for(int i=0;i<D;i++)D_cost[i]=0;
	Act_t=m_iarr1(FMStep+1);
	EM_gzai1=m_darr1(TimeStep);
	EM_gzai2=m_darr1(TimeStep);
	EM_ux1=m_darr1(TimeStep);
	EM_ux2=m_darr1(TimeStep);
	EM_uy1=m_darr1(TimeStep);
	EM_uy2=m_darr1(TimeStep);
	EM_sigma2=m_darr1(TimeStep);
	EM_sigmaxy1=m_darr1(TimeStep);
	EM_sigmaxy2=m_darr1(TimeStep);

	//入札パターンの作成//
	make_bidpat();
	make_loadpat(day,&loadpat);
	loadpat=0;

	//開始時間まで待機//
	alarm();
	//プログラム開始//
	Start=(double)clock();
	
	int kaishi;
	
	while(1){
		//間隔をランダムに決める//
		kan=t_dis();
		bidtime+=kan;
		if(bidtime>Byo){//次の動作が次の日になる時
			bidtime-=Byo;
			printf("%d日目のコストは%lf円でした。\n",day,D_cost[day]);
			result4(day,&tra_n,shor,cost,&rec_n,battery,bat);
			result5(day,prob_m2);
			day++;
			make_loadpat(day,&loadpat);
			printf("day%d\n",day);
			k=0;
			if(day==D){
				break;//日を超えると終了日になる時
				DropTable(&henv, &hdbc, &hstmt); // テーブルの削除
				Close(&henv,&hdbc,&hstmt); // 切断
			}
		}
		t_cost=0;

		step[day][k]=int(bidtime/(Byo/FMStep));
		while(d_sec(day)<bidtime){//次の動作を待つ間も以下の動作は行う
			get_keep1(day,&baff,&bat,&t_cost,Pe[step[day][k]],&rec_n,Pe[int(d_sec(day)/(Byo/FMStep))],EM_p,ykj,henv,hdbc,hstmt);
		}
		if(k==0)kaishi=clock();
		step[day][k]=int(bidtime/(Byo/FMStep));

		//時間が来ると注文を取り下げる//
		printf("注文が市場に出てる時間　%d\n",(int)clock()-kaishi);
		cancel(day,bnum,branks,henv,hdbc,hstmt);
		get_keep2(day,henv,hdbc,hstmt);
		get_rec(day,&baff,&bat,&t_cost,&rec_n,Pe[step[day][k]],EM_p,ykj,henv,hdbc,hstmt);
		
		while(agree_cancel(day,bnum,branks,henv,hdbc,hstmt)){
			get_keep2(day,henv,hdbc,hstmt);
			get_rec(day,&baff,&bat,&t_cost,&rec_n,Pe[step[day][k]],EM_p,ykj,henv,hdbc,hstmt);
		}		
		cost[k]=t_cost;
		
		//lc_elec[day][k]=kan/(Byo/FMStep)*Lc_elec[step[day][k]];
		lc_elec[day][k]=kan/(Byo/FMStep)*Lc_elecs[loadpat][step[day][k]];
		//action//
		action(step[day][k],&baff,&bat,&(shor[k]),&(cost[k]),Pe[step[day][k]],lc_elec[day][k]);
		
		int time=(int)floor((double)d_sec(day)/(Byo/TimeStep));
		if(time>=24)time-=TimeStep;
		readfile4(&(m[day][k]),henv,hdbc,hstmt);
		//EM_p[day][time] = m[day][k];
		//calprobabilityB(k,day,step[day][k],m,trans_m,prob_m,P_num);

		EMalgorithm(day,step[day][k],ux,uy,gzai,sigma2,EM_p,prob_m2,sigmaxy,ykj);

		printf("\nStochasticDynamicProgramming\n");
		kaishi=clock();
		//int time=(int)floor((double)d_sec(day)/(Byo/TimeStep));
		//if(time>=24)time-=TimeStep;
		//StochasticDynamicProgramming0(prob_m,q_table,a_table);
		//StochasticDynamicProgramming1(time,prob_m,q_table,a_table);
		StochasticDynamicProgramming1(time,loadpat,prob_m2,q_table,a_table);

		printf("確率動的計画法の時間　%d\n\n",(int)clock()-kaishi);

		//行動の決定//
		//入札//
		what_time(d_sec(day),day);
		
		time=(int)floor((double)d_sec(day)/(Byo/TimeStep));
		if(time>=24)time-=TimeStep;
		bi(day,bat,time,m[day][k],&bnum,&tra_n,a_table,henv,hdbc,hstmt);
		agree_order(bnum,&branks,henv,hdbc,hstmt);
		
		kaishi=clock();

		Act_t[k]=(int)d_sec(day);
		battery[k]=bat;
		D_cost[day]+=cost[k];
		k++;
	}
}
void  connectmarket(){
	FILE *fp;
	fp = fopen("../../System/connect market.dat","r");	
	while(getc(fp)!=':');
	fscanf(fp,"%d",&(M_N));	

	char str1[256],str2[256];
	int i=0;
	while(1){
		sprintf(str1,"../../System/MIKAWA/f/inoue/market%d/%d/name.dat",M_N,i);
		sprintf(str2,"../../System/MIKAWA/f/inoue/market%d/%d",M_N,i);
		if((fp = fopen(str1,"r"))==NULL){
			_mkdir(str2);
			sprintf(str1,"../../System/MIKAWA/f/inoue/market%d/%d/name.dat",M_N,i);
			do fp = fopen(str1,"w");
			while(fp==NULL);
			fprintf(fp,"%s",Name);
			fclose(fp);
			ID=i;
			break;
		}
		else fclose(fp);
		i++;
	}
	int j=0;
	while(1){
		sprintf(str1,"../../System/result/EMAlgorithm/ContractPrice/%d.dat",j);
		if((fp=fopen(str1,"r"))==NULL){
			Sta_d=j;
			break;
		}
		fclose(fp);
		j++;
	}
}

void con(){
	D=11;
	Load_cuvs=20;
	FMStep=48;
	BatStep=10;
	TimeStep=24;
	Scale=125;
	Byo=86400;

	Max_b=5;//一回当たりの最大入札数

	M_dev1=5;
	M_dev2=2.5;
	T_dev=200;
	Name='1';

	ux_day1 = 8.0;
	ux_day2 = 2.0;
	ux_night1 = 7.0;
	ux_night2 = 1.0;
	uy_day1 = 8.0;
	uy_day2 = 2.0;
	uy_night1 = 7.0;
	uy_night2 = 1.0;

	ini = 60;
	ykjMAX=150;
	sigma2=5.0;
	sigmaxy[0]=0.5;sigmaxy[1]=0.5;
	gzai[0]=0.5;gzai[1]=0.5;
}

void init_prob2(double **EM_p,double ****p_m,int*ykj){
	FILE *fp;
	char str[256];
	double ***temp,sum;
	double r1,r2;
	int ykjtmp=0;
	temp = m_darr3(TimeStep,P_num,P_num);
	srand((unsigned)time(NULL));

	if(Sta_d==0){
		for(int i=0; i<TimeStep; i++)ykj[i]=ini;
		for(int i=0; i<TimeStep; i++){
			for(int j=0; j<ini; j++){
				r1=(double)rand()/(RAND_MAX+1);
				r2=(double)rand()/(RAND_MAX+1);
				if( i<7 || i>22 ){
					if(j<ini/2) EM_p[i][j] = (ux_night1+sqrt(sigma2)*sqrt(-2*log(r1))*cos(2*3.1415*r2));
					else EM_p[i][j] = (ux_night2+sqrt(sigma2)*sqrt(-2*log(r1))*cos(2*3.1415*r2));
				}
				else{
					if(j<ini/2) EM_p[i][j] = (ux_day1+sqrt(sigma2)*sqrt(-2*log(r1))*cos(2*3.1415*r2));
					else EM_p[i][j] = (ux_day2+sqrt(sigma2)*sqrt(-2*log(r1))*cos(2*3.1415*r2));
				}
			}
			for(int j=ini; j<ykjMAX; j++){
				EM_p[i][j]=0;
			}
		}
		for(int k=0; k<TimeStep; k++){
			for(int i=0; i<P_num; i++){
				for(int j=0; j<P_num; j++){
					if( k>6 && k<23){
						ux[0]=ux_day1;ux[1]=ux_day2;uy[0]=uy_day1;uy[1]=uy_day2;
					}
					else{
						ux[0]=ux_night1;ux[1]=ux_night2;uy[0]=uy_night1;uy[1]=uy_night2;
					}
					temp[k][i][j] = gzai[0] / (2*3.1415*sigma2) / sqrt( 1 - sigmaxy[0] * sigmaxy[0] ) * exp(-0.5 / ( 1 - sigmaxy[0] * sigmaxy[0] ) * ( pow((i-ux[0]),2.0)/sigma2 + pow((j-uy[0]),2.0)/sigma2 - 2*sigmaxy[0]*(i-ux[0])*(j-uy[0])/sigma2)) + gzai[1] / (2*3.1415*sigma2) / sqrt(1-sigmaxy[1]*sigmaxy[1]) * exp( -0.5 / (1-sigmaxy[1]*sigmaxy[1]) * (pow((i-ux[1]),2.0)/sigma2 + pow((j-uy[1]),2.0)/sigma2 - 2*sigmaxy[1]*(i-ux[1])*(j-uy[1])/sigma2 ));
				}
			}
		}

		for(int k=0; k<TimeStep; k++){
			for(int i=0; i<P_num; i++){
				sum=0;
				for(int j=0; j<P_num; j++) sum += temp[k][i][j];
				for(int j=0; j<P_num; j++) (*p_m)[k][i][j] = temp[k][i][j] / sum;
			}
		}
		for (int k=0 ;k<TimeStep; k++){
			for (int i=0; i<P_num; i++){
				free(temp[k][i]);
			}
			free(temp[k]);
		}
		free(temp);
	}
	else{
		for(int i=0; i<TimeStep; i++){
			for(int j=0; j<ykjMAX; j++){
				EM_p[i][j] = 0;
			}
		}
		sprintf(str,"../../System/result/EMAlgorithm/ykj/%d.dat",Sta_d-1);
		if((fp = fopen(str,"r"))==NULL){
			printf("%sが開けません",str);
			exit(0);
		}
		while(getc(fp) != '\n');
		for(int i=0; i<TimeStep; i++){
			fscanf(fp,"%d",&ykj[i]);
			while(getc(fp) != '\n');
		}
		for(int i=0; i<TimeStep; i++){
			if(ykjtmp < ykj[i]) ykjtmp = ykj[i];
		}
		if(ykjtmp>ykjMAX) ykjtmp=ykjMAX;
		sprintf(str,"../../System/result/EMAlgorithm/ContractPrice/%d.dat",Sta_d-1);
		if((fp = fopen(str,"r"))==NULL){
			printf("%sが開けません",str);
			exit(0);
		}
		for(int i=0; i<TimeStep; i++){
			while(getc(fp) != '\n');
			for(int j=0; j<ykjtmp; j++){
				fscanf(fp,"%lf",&(EM_p[i][j]));
				while(getc(fp) != ',');
			}
		}
		fclose(fp);
		sprintf(str,"../../System/result/EMAlgorithm/TransitionProbability/%d.dat",Sta_d-1);
		if((fp = fopen(str,"r"))==NULL){
			printf("%sが開けません",str);
			exit(0);
		}
		for(int i=0;i<TimeStep;i++){
			for(int j=0;j<P_num;j++){
				while(getc(fp)!='\n');
				for(int k=0;k<P_num;k++){
					fscanf(fp,"%lf",&((*p_m)[i][j][k]));
					while(getc(fp)!=',');
				}
			}
			while(getc(fp)!='\n');
		}
		fclose(fp);
	}
}
void make_bidpat(){
	int i,j,k,l,j0,k0,l0;
	//bidpatternをつくる.bidpatternは三角形。売買、底辺、高さの３要素からなる。//
	Bidpat=0;
	for(i=0;i<Max_b;i++)Bidpat+=(Max_b-i)*(P_num-i);//iは三角形の高さ-1
	Bidpat*=2;
	Bids=m_iarr2(Bidpat,P_num);
	for(i=0;i<2;i++){//売買
		for(j=0;j<Max_b;j++){//高さ-1
			for(k=Max_b-1;k>=j;k--){//底辺-1
				for(l=0;l<P_num-j;l++){//頂点（下の端）の位置
					//今の形が何番目かを計算//
					int b_num=0;
					if(i==1)b_num+=Bidpat/2;//売買
					for(j0=0;j0<j;j0++){//高さ-1
						b_num+=(P_num-j0)*(Max_b-j0);
					}
					for(k0=Max_b-1;k0>k;k0--)b_num+=P_num-j;//底辺の長さ-1
					b_num+=l;//頂点の位置
					if(i==0){//売りの時
						for(l0=0;l0<l;l0++)Bids[b_num][l0]=0;
						for(l0=l;l0<l+j+1;l0++){
							Bids[b_num][l0]=(int)floor(((double)k+1)/(j+1)*(l0-l+1));							
						}
						for(l0=l+j+1;l0<P_num;l0++)Bids[b_num][l0]=0;
					}
					if(i==1){//買いの時
						for(l0=0;l0<l;l0++)Bids[b_num][l0]=0;
						for(l0=l;l0<l+j+1;l0++){
							Bids[b_num][l0]=-(int)floor(((double)k+1)/(j+1)*(l+j-l0+1));
						}
						for(l0=l+j+1;l0<P_num;l0++)Bids[b_num][l0]=0;
					}
				}
			}
		}
	}
}
void make_loadpat(int d,int *lp){
	srand((unsigned)time(NULL));
	*lp=(int)(Load_cuvs*(double)rand()/RAND_MAX);
	
	FILE *fp;
	char str[256];
	sprintf(str,"../../System/result/LoadCurves/%d.csv",d);
	do fp = fopen(str,"w");
	while(fp==NULL);
	for(int i=0;i<FMStep;i++){
		fprintf(fp,"%lf,",Lc_elecs[*lp][i]);
	}
	fclose(fp);
}
void alarm()
{ 
	time_t timer;
	struct tm *t_time;
    int in_hour;
	int in_min;
	int in_sec;
	while(1){
		/* 時刻取得，表示 */
		time(&timer);                   // 現在の時刻取得
		printf("現在%s\n", ctime(&timer));  // 現在時刻表示
		
		/* 入力 */
		printf("開始させる時刻を入力してください。\n");
		printf("時[0-23]>");
		scanf("%d",&in_hour);
		printf("分[0-59]>");
		scanf("%d",&in_min);
		printf("秒[0-59]>");
		scanf("%d",&in_sec);

		time(&timer);  
		t_time = localtime(&timer);
		int al,now;
		al=in_hour*3600+in_min*60+in_sec;
		now=t_time->tm_hour*3600+t_time->tm_min*60+t_time->tm_sec;
		if(al<now){
			printf("予約時刻は過ぎています\n\n\n\n");
		}
		else break;
	}
	
	printf("待機中･･･\n");
	
	while(1){
		t_time = localtime(&timer);
		if(in_hour==t_time->tm_hour && in_min==t_time->tm_min && in_sec==t_time->tm_sec){
			printf("時間になりました\n");
			break;
		}
		else{
			time(&timer);// 現在の時刻再取得
			t_time = localtime(&timer);
		}
	}
}

double dis(){
	srand((unsigned)time(NULL));
	double v1=0, u1=0, u2=0;
	double v2=0, s=0,x=0;
	do{
		u1 = (double)rand()/RAND_MAX;
		u2 = (double)rand()/RAND_MAX;
		v1 = 2.0 * u1 - 1.0;
		v2 = 2.0 * u2 - 1.0;
		s = v1 * v1 + v2 * v2;
	}
	while(s >= 1.0 || s == 0.0);
	x = v1 * sqrt(-2 * log(s)/s);
	return x;
}

double t_dis(){
	double kan=dis()*T_dev+Byo/FMStep;
	return kan;
}


double d_sec(int da){
	double s_c,a;
	a=(double)clock();
	s_c=(((double)clock()-Start)/1000)*Scale-da*Byo;
	return s_c;
}
void what_time(double s,int d){
	int hour,min;
	double sec;
	hour=(int)s/3600;
	min=((int)s-hour*3600)/60;
	sec=s-hour*3600-min*60;
	printf("%d日目%d時%d分%.1lf秒入札\n",d+Sta_d,hour,min,sec);
}
void get_keep1(int d,double *baf,double *bat,double *cost,double pe,int *rec_n,double pena,double**EM_p,int*ykj, HENV henv, HDBC hdbc,HSTMT hstmt){
	int rank,time,bs,p,q;
	DWORD re_KeepRank,re_KeepTime,re_BuyOrSell,re_OrderPrice,re_KeepQuantity;
	int SQLline=0;

	_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM keep_suc%d"),ID);
	Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

	if( SQLline != 0){
		_stprintf_s(SQLbuff,buffsize,TEXT("SELECT KeepRank,KeepTime,BuyOrSell,OrderPrice,KeepQuantity FROM keep_suc%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
		SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_KeepRank, (SDWORD)sizeof(re_KeepRank), NULL);
		SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_KeepTime, (SDWORD)sizeof(re_KeepTime), NULL);
		SQLBindCol(hstmt, 3, SQL_C_SLONG, &re_BuyOrSell, (SDWORD)sizeof(re_BuyOrSell), NULL);
		SQLBindCol(hstmt, 4, SQL_C_SLONG, &re_OrderPrice, (SDWORD)sizeof(re_OrderPrice), NULL);
		SQLBindCol(hstmt, 5, SQL_C_SLONG, &re_KeepQuantity, (SDWORD)sizeof(re_KeepQuantity), NULL);
		while(1){
			RETCODE rc = SQLFetch(hstmt);
			if( rc == SQL_NO_DATA_FOUND ) break;
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
				printf("ERR\n");
				break;
			}
			rank = re_KeepRank;
			time = re_KeepTime;
			bs = re_BuyOrSell;
			p = re_OrderPrice;
			q = re_KeepQuantity;
			what_time(d_sec(d),d);
			printf("%.1lf円/kWhの注文%.1lfWhをkeepできました\n",(Min_p+p*Unit_p)*1000,q*Unit_bid);
			
			SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
			_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO get%d(OrderPrice,OrderQuantity,OrderRank) VALUES(%d,%d,%d)"),ID,p,q,rank);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);
			SQLTransact( henv, hdbc, SQL_COMMIT );
			SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
			what_time(d_sec(d),d);
			printf("%.1lf円/kWhの注文%.1lfWhをgetします\n",(Min_p+p*Unit_p)*1000,q*Unit_bid);
			get_rec(d,baf,bat,cost,rec_n,pe,EM_p,ykj,henv,hdbc,hstmt);
		}
		SQLFreeStmt(hstmt, SQL_DROP);
		SQLAllocStmt(hdbc, &hstmt);
		_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE keep_suc%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
	}
}
void get_keep2(int d, HENV henv, HDBC hdbc,HSTMT hstmt){
	int rank,time,bs,p,q;
	DWORD re_KeepRank,re_KeepTime,re_BuyOrSell,re_OrderPrice,re_KeepQuantity;
	int SQLline=0;

	_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM keep_suc%d"),ID);
	Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

	if( SQLline != 0){
		_stprintf_s(SQLbuff,buffsize,TEXT("SELECT KeepRank,KeepTime,BuyOrSell,OrderPrice,KeepQuantity FROM keep_suc%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);	
		SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_KeepRank, (SDWORD)sizeof(re_KeepRank), NULL);
		SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_KeepTime, (SDWORD)sizeof(re_KeepTime), NULL);
		SQLBindCol(hstmt, 3, SQL_C_SLONG, &re_BuyOrSell, (SDWORD)sizeof(re_BuyOrSell), NULL);
		SQLBindCol(hstmt, 4, SQL_C_SLONG, &re_OrderPrice, (SDWORD)sizeof(re_OrderPrice), NULL);
		SQLBindCol(hstmt, 5, SQL_C_SLONG, &re_KeepQuantity, (SDWORD)sizeof(re_KeepQuantity), NULL);
		while(1){
			RETCODE rc = SQLFetch(hstmt);
			if( rc == SQL_NO_DATA_FOUND ) break;
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
				printf("ERR\n");
				break;
			}
			rank = re_KeepRank;
			time = re_KeepTime;
			bs = re_BuyOrSell;
			p = re_OrderPrice;
			q = re_KeepQuantity;
			what_time(d_sec(d),d);
			printf("%.1lf円/kWhの注文%.1lfWhをkeepできました\n",(Min_p+p*Unit_p)*1000,q*Unit_bid);
		}
		SQLFreeStmt(hstmt, SQL_DROP);
		SQLAllocStmt(hdbc, &hstmt);
		_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE keep_suc%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
	}
}
void get_rec(int d,double *baf,double *bat,double *cost,int *rec_n,double pena,double**em,int*ykj, HENV henv, HDBC hdbc,HSTMT hstmt){
	int i=0;
	int h,ykjtmp;
	int bs=0,or_p=0,p=0,q=0,time=0,y=0;
	DWORD re_AgreeTime,re_BuyOrSell,re_OrderPrice,re_AgreePrice,re_AgreeQuantity;
	int SQLline;
		
	_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM rec%d"),ID);
	Count(henv, hdbc, hstmt, &SQLline, SQLbuff);
		
	if( SQLline != 0 ){
		_stprintf_s(SQLbuff,buffsize,TEXT("SELECT AgreeTime,BuyOrSell,OrderPrice,AgreePrice,AgreeQuantity FROM rec%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);	
		SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_AgreeTime, (SDWORD)sizeof(re_AgreeTime), NULL);
		SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_BuyOrSell, (SDWORD)sizeof(re_BuyOrSell), NULL);
		SQLBindCol(hstmt, 3, SQL_C_SLONG, &re_OrderPrice, (SDWORD)sizeof(re_OrderPrice), NULL);
		SQLBindCol(hstmt, 4, SQL_C_SLONG, &re_AgreePrice, (SDWORD)sizeof(re_AgreePrice), NULL);
		SQLBindCol(hstmt, 5, SQL_C_SLONG, &re_AgreeQuantity, (SDWORD)sizeof(re_AgreeQuantity), NULL);
		while(1){
			RETCODE rc = SQLFetch(hstmt);
			if( rc == SQL_NO_DATA_FOUND ) break;
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
				printf("ERR\n");
				break;
			}
			time = re_AgreeTime;
			bs = re_BuyOrSell;
			or_p = re_OrderPrice;
			p = re_AgreePrice;
			q = re_AgreeQuantity;

			*baf-=q*Unit_bid;
			*cost-=(q*Unit_bid)*(Min_p+p*Unit_p);
			(*rec_n)++;
			Agree_t=(int*)realloc(Agree_t,sizeof(int)*(*rec_n+1));
			Agree_ord_p=(double*)realloc(Agree_ord_p,sizeof(double)*(*rec_n+1));
			Agree_p=(double*)realloc(Agree_p,sizeof(double)*(*rec_n+1));
			Agree_q=(double*)realloc(Agree_q,sizeof(double)*(*rec_n+1));
			what_time(d_sec(d),d);
			printf("%.1lf円/kWhで%.1lf円/kWhの注文が%.1lfWh約定しました\n",(Min_p+p*Unit_p)*1000,(Min_p+or_p*Unit_p)*1000,q*Unit_bid);

			h = (int)floor((double)d_sec(d)/(Byo/TimeStep));
			if(h >= 24) h -= TimeStep;
			ykj[h] += 1;
			if(ykj[h] > ykjMAX){
				ykjtmp = ykj[h] % ykjMAX + ini - 1;
				em[h][ykjtmp] = p;
			}
			else em[h][ykj[h]-1] = p;

			Agree_t[*rec_n]=time;
			Agree_ord_p[*rec_n]=Min_p+or_p*Unit_p;
			Agree_p[*rec_n]=Min_p+p*Unit_p;
			Agree_q[*rec_n]=-q*Unit_bid;
		}
		SQLFreeStmt(hstmt, SQL_DROP);
		SQLAllocStmt(hdbc, &hstmt);
		_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE rec%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
	}

	if(*baf<0){
		if(*bat*Bat_eff[2]>=-(*baf)){
			*bat+=(*baf)/Bat_eff[2];
			*baf=0;
		}
		else{
			*baf+=*bat*Bat_eff[2];
			*bat=0;
			*cost-=(*baf)*pena;
			*baf=0;
		}
	}
}

void StochasticDynamicProgramming1(int t,int lp,double***p_m,double ***q_table,int ***a_table){
	int ii,i,j,k,j0,k0,m0,n,slot;
	int n_opt,flag;
	double qmin,v,dq,dq0,ddq,bat,Q;
	double *TempQ;
	
	TempQ=m_darr1(Bidpat);

	for(i=0;i<TimeStep;i++){
		for(j=0;j<BatStep;j++){
			for(k=0;k<P_num;k++){
				q_table[i][j][k]=0.;
				a_table[i][j][k]=-1;
			}
		}
	}

	slot=FMStep/TimeStep;
	dq0=0.;
	n=0;
	do{
		for(ii=t+TimeStep-1;ii>=t;ii--){
			if(ii>=TimeStep)i=ii-TimeStep;
			else i=ii;
			flag=1;
			dq=0.;
			for(j=0;j<BatStep;j++){
				for(k=0;k<P_num;k++){
					for(m0=0;m0<Bidpat;m0++){
						TempQ[m0]=0.;
						for(k0=0;k0<P_num;k0++){
							if(p_m[i][k][k0]>0.001){
								calc_Q(i,j,lp,&j0,k,k0,m0,&Q,&bat);
								if(j0==BatStep-1){
									if(i==TimeStep-1)Q+=q_table[0][j0][k0];
									else Q+=q_table[i+1][j0][k0];
								}
								else{
									v=(bat-UnitBat*j0)/UnitBat;
									if(i==TimeStep-1)Q+=(1.0-v)*q_table[0][j0][k0]+v*q_table[0][j0+1][k0];
									else Q+=(1.0-v)*q_table[i+1][j0][k0]+v*q_table[i+1][j0+1][k0];
								}
								TempQ[m0]+=Q*p_m[i][k][k0];

							}
						}
					}
					n_opt=-1;
					qmin=1.e9;
					for(m0=0;m0<Bidpat;m0++){
						if(qmin>TempQ[m0]){
							qmin=TempQ[m0];
							n_opt=m0;
						}
					}
					if(n_opt>=0&&n_opt<Bidpat){
						if(a_table[i][j][k]!=n_opt){
							flag=0;//a_tableが更新されたflag
						}
						a_table[i][j][k]=n_opt;
						v=qmin-q_table[i][j][k];
						dq+=v*v;
						q_table[i][j][k]=qmin;
						}
					else {
						
						printf("Error in Stochastic Dynamic Programming!!!\n");
						exit(0);
					}
				}
			}
		}
		ddq=fabs(dq0-dq);
		printf(" Iteration %d , Daily Cost= %f\n",++n,v);
		dq0=dq;
	}
	while(flag==0||ddq/dq>0.01);
	free(TempQ);
}
void calc_Q(int i,int j,int lp,int *j0,int k,int k0,int m0,double *Q,double*bat){
	double d,e,g,pe,c,t;
	d=0;g=0;pe=0;c=0;t=0;

	for(int x=i*FMStep/TimeStep;x<(i+1)*FMStep/TimeStep;x++){
		d+=Lc_elecs[lp][x];
		g+=PVout[x];
		pe+=Pe[x];
	}
	d/=(FMStep/TimeStep);
	g/=(FMStep/TimeStep);
	pe/=(FMStep/TimeStep);
	
	//以下取引分//
	if(m0<Bidpat/2){//売り
		for(int x=0;x<k0+1;x++){
			c-=(Min_p+x*Unit_p)*(Bids[m0][x]*Unit_bid);
			t-=Bids[m0][x]*Unit_bid;
		}
	}
	else if(Bidpat/2<=m0 && m0<Bidpat){//買い
		for(int x=k0;x<P_num;x++){
			c-=(Min_p+x*Unit_p)*(Bids[m0][x]*Unit_bid);
			t-=Bids[m0][x]*Unit_bid;
		}
	}
	else{
		printf("Error in calcQ");
		exit(0);
	}
	e=g-d+t;
	*bat=UnitBat*j;
	if(e<0){
		if(-e>=*bat*Bat_eff[2]){//バッテリーの電気を使っても不足する場合
			c+=(-e-*bat*Bat_eff[2])*pe;
			*bat=0;
		}
		else if(-e<*bat*Bat_eff[2]){//バッテリーの電気を使えば補える場合
			*bat+=e/Bat_eff[2];
		}
	}
	else{
		*bat+=e*Bat_eff[1];
	}

	if(*bat>Batmax){
		if( i>6 && i<23 ) c -= (*bat - Batmax) * 0.011;
		else c -= ( *bat - Batmax) * 0.005;
      *bat=Batmax;
      *j0=BatStep-1;
    }
    else if(*bat<=Batmax){
      *bat=*bat*pow(Bat_eff[0],(FMStep/TimeStep));
      *j0=(int)floor(*bat/UnitBat);
    }
	*Q=c;
}
void bi(int d,double ba,int t,int m,int *bnum,int *tra_n,int ***a_table, HENV henv, HDBC hdbc,HSTMT hstmt){
	int p,bpat;
	*bnum=0;

	if(ba==Batmax)p=BatStep-1;
	else p=(int)floor(ba/UnitBat);
	bpat=a_table[t][p][m];

	if(bpat<Bidpat/2){
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
		for(int i=0; i<P_num; i++){
			if(Bids[bpat][i] != 0){
				_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO keep%d(OrderPrice,OrderQuantity) VALUES(%d,%d)"),ID,i,Bids[bpat][i]);
				Execute(&henv,&hdbc,&hstmt,SQLbuff);
				what_time(d_sec(d),d);
				printf("%.1lf円/kWhで%.1lfWhをbid\n",(Min_p+i*Unit_p)*1000,Bids[bpat][i]*Unit_bid);
			}
		}
		SQLTransact( henv, hdbc, SQL_COMMIT );
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	}
	else{
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
		for(int i=P_num-1; i >= 0; i--){
			if(Bids[bpat][i] != 0){
				_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO keep%d(OrderPrice,OrderQuantity) VALUES(%d,%d)"),ID,i,Bids[bpat][i]);
				Execute(&henv,&hdbc,&hstmt,SQLbuff);
				what_time(d_sec(d),d);
				printf("%.1lf円/kWhで%.1lfWhをbid\n",(Min_p+i*Unit_p)*1000,Bids[bpat][i]*Unit_bid);
			}
		}
		SQLTransact( henv, hdbc, SQL_COMMIT );
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);

	}
	printf("\n");
	(*tra_n)++;

	Tra_t=(int*)realloc(Tra_t,sizeof(int)*(*tra_n+1));
	Trading=ex_iarr2(Trading,*tra_n);
	for(int i=0;i<P_num;i++){
		Trading[*tra_n][i]=Bids[bpat][i];
	}
	Tra_t[*tra_n]=(int)d_sec(d);
}
void agree_order(int bnum,int**branks, HENV henv, HDBC hdbc,HSTMT hstmt){
	(*branks)=m_iarr1(bnum);
	int flag=0;
	DWORD re_EnterRank;
	int SQLline=0;

	_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM enter%d"),ID);
	Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

	if( SQLline != 0 ){
		_stprintf_s(SQLbuff,buffsize,TEXT("SELECT EnterRank FROM enter%d"),ID);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);	
		SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_EnterRank, (SDWORD)sizeof(re_EnterRank), NULL);
		while(1){
			RETCODE rc = SQLFetch(hstmt);
			if( rc == SQL_NO_DATA_FOUND ) break;
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
				printf("ERR\n");
				break;
			}
			*((*branks)+flag) = re_EnterRank;
			flag++;
		}
	}
	SQLFreeStmt(hstmt, SQL_DROP);
	SQLAllocStmt(hdbc, &hstmt);
	_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE enter%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
}
void cancel(int d,int bnum,int*branks, HENV henv, HDBC hdbc,HSTMT hstmt){
	if(bnum!=0){
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
		for(int k=0;k<bnum;k++){
			_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO can%d(NumberOfCancel,Rank) VALUES(%d,%d)"),ID,bnum,branks[k]);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);
		}
		SQLTransact( henv, hdbc, SQL_COMMIT );
		SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
		what_time(d_sec(d),d);
		printf("cancel発注\n");
	}
}
int agree_cancel(int d,int bnum,int *branks, HENV henv, HDBC hdbc,HSTMT hstmt){
	if(bnum==0)return(0);
	else{
		int SQLline=0;
		_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM cancel_agree%d"),ID);
		Count(henv, hdbc, hstmt, &SQLline, SQLbuff);
		if( SQLline != 0 ){
			what_time(d_sec(d),d);
			printf("キャンセルを受領されました\n");
			_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE cancel_agree%d"),ID);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);
			free(branks);
			return(0);
		}
		else return(1);
	}
}
void action(int ste,double *baf,double *bat,double *sho,double *co,double pena,double lcele){
	*bat *= Bat_eff[0];
	*baf += PVout[ste];
	if( *baf >= lcele){
		*baf -= lcele;
		*bat += *baf * Bat_eff[1];
		if( *bat > Batmax ){
			if( ste>13 && ste<46 ) *co -= (*bat - Batmax) * 0.011;
			else *co -= ( *bat - Batmax) * 0.005;
			*bat = Batmax;
		}
		*baf=0;
		*sho=0;
	}
	else{
		lcele -= *baf;
		*baf=0;
		if( *bat * Bat_eff[2] >= lcele){
			*bat -= lcele/Bat_eff[2];
			if( *bat > Batmax)*bat=Batmax;
			*sho=0;
		}
		else{
			*sho=lcele-*bat*Bat_eff[2];
			*bat=0;
		}
	}
	*co += *sho*pena;
}


void EMalgorithm(int d,int step, double *ux, double *uy,double *gzai,double sigma2, double **em,double ***pro,double*sigmaxy,int*ykj){
	int h,tmpykj,k=0;
	double s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12;
	double **f,**z,**temp,sum;

	h=(int)floor((double)d_sec(d)/(Byo/TimeStep));
	if(h >= 24) h -= TimeStep;
	if(ykj[h] > ykjMAX) tmpykj=ykjMAX;
	else tmpykj = ykj[h];

	temp=m_darr2(P_num,P_num);
	f=m_darr2(tmpykj,2);
	z=m_darr2(tmpykj,2);

	if( h<7 || h>22){
		ux[0]=ux_night1;ux[1]=ux_night2;uy[0]=uy_night1;uy[1]=uy_night2;
	}
	else{
		ux[0]=ux_day1;ux[1]=ux_day2;uy[0]=uy_day1;uy[1]=uy_day2;
	}
	for(int i=1; i<tmpykj; i++){
		for(int j=0; j<2; j++){
			f[i][j] = 1 / (2*3.1415*sigma2) / sqrt(1-sigmaxy[j]*sigmaxy[j]) * exp( -1 / 2 / (1-sigmaxy[j]*sigmaxy[j]) * ( pow((em[h][i]-ux[j]),2.0) / sigma2 + pow((em[h][i-1]-uy[j]),2.0) / sigma2 - 2*sigmaxy[j]*(em[h][i]-ux[j])*(em[h][i-1]-uy[j])/sigma2 ));
		}
	}

	for(int i=1; i<tmpykj; i++){
		for(int j=0; j<2; j++){
			z[i][j] = gzai[j] * f[i][j] / ( gzai[0]*f[i][0] + gzai[1]*f[i][1] );
		}
	}

	for(int k=0; k<10; k++){
		s1=0;s2=0;s3=0;s4=0;s5=0;s6=0;s7=0;s8=0;s9=0;s10=0;s11=0;s12=0;sigma2=0;
		for(int i=1; i<tmpykj; i++){
			s1 += z[i][0];
			s2 += z[i][1];
			s3 += z[i][0] * em[h][i];
			s4 += z[i][1] * em[h][i];
			s5 += z[i][0] * em[h][i-1];
			s6 += z[i][1] * em[h][i-1];
		}

		ux[0] = s3/s1;
		ux[1] = s4/s2;
		uy[0] = s5/s1;
		uy[1] = s6/s2;

		for(int i=1; i<tmpykj; i++){
			s7 += (em[h][i] - ux[0]) * (em[h][i-1] - uy[0]);
			s8 += pow((em[h][i] - ux[0]),2.0);
			s9 += pow((em[h][i-1] - uy[0]),2.0);
			s10 += (em[h][i] - ux[1]) * (em[h][i-1] - uy[1]);
			s11 += pow((em[h][i] - ux[1]),2.0);
			s12 += pow((em[h][i-1] - uy[1]),2.0);
		}
		sigmaxy[0] = s7 / sqrt(s8) / sqrt(s9);
		sigmaxy[1] = s10 / sqrt(s11) / sqrt(s12);

		for(int i=1; i<tmpykj; i++){
			for (int j=0; j<2; j++){
				sigma2 += z[i][j] / ( 2 * tmpykj * (1-sigmaxy[j]*sigmaxy[j]) ) * ( pow( (em[h][i] - ux[j]) ,2.0) + pow( (em[h][i-1] - uy[j]) ,2.0) - 2 * sigmaxy[j] * (em[h][i]-ux[j]) * (em[h][i-1]-uy[j]) );
			}
		}
		gzai[0] = s1 / (tmpykj);
		gzai[1] = 1.0 - gzai[0];
		for(int i=1; i<tmpykj; i++){
			for(int j=0; j<2; j++){
			f[i][j] = 1 / (2*3.1415*sigma2) / sqrt(1-sigmaxy[j]*sigmaxy[j]) * exp( -1 / 2 / (1-sigmaxy[j]*sigmaxy[j]) * ( pow((em[h][i]-ux[j]),2.0) / sigma2 + pow((em[h][i-1]-uy[j]),2.0) / sigma2 - 2*sigmaxy[j]*(em[h][i]-ux[j])*(em[h][i-1]-uy[j])/sigma2 ));
			}
		}

		for(int i=1; i<tmpykj; i++){
			for(int j=0; j<2; j++){
				z[i][j] = gzai[j] * f[i][j] / ( gzai[0]*f[i][0] + gzai[1]*f[i][1] );
			}
		}
		k++;
	}
		
	for(int i=0; i<P_num; i++){
		for(int j=0; j<P_num; j++){
			temp[i][j] = gzai[0] / (2*3.1415*sigma2) / sqrt( 1 - sigmaxy[0] * sigmaxy[0] ) * exp(-0.5 / ( 1 - sigmaxy[0] * sigmaxy[0] ) * ( pow((i-ux[0]),2.0)/sigma2 + pow((j-uy[0]),2.0)/sigma2 - 2*sigmaxy[0]*(i-ux[0])*(j-uy[0])/sigma2)) + gzai[1] / (2*3.1415*sigma2) / sqrt(1-sigmaxy[1]*sigmaxy[1]) * exp( -0.5 / (1-sigmaxy[1]*sigmaxy[1]) * (pow((i-ux[1]),2.0)/sigma2 + pow((j-uy[1]),2.0)/sigma2 - 2*sigmaxy[1]*(i-ux[1])*(j-uy[1])/sigma2 ));
		}
	}
	for(int i=0; i<P_num; i++){
		sum=0;
		for(int j=0; j<P_num; j++) sum += temp[i][j];
		for(int j=0; j<P_num; j++) pro[h][i][j] = temp[i][j] / sum;
	}
	EM_gzai1[h]=gzai[0];
	EM_gzai2[h]=gzai[1];
	EM_ux1[h]=ux[0];
	EM_ux2[h]=ux[1];
	EM_uy1[h]=uy[0];
	EM_uy2[h]=uy[1];
	EM_sigma2[h]=sigma2;
	EM_sigmaxy1[h]=sigmaxy[0];
	EM_sigmaxy2[h]=sigmaxy[1];

	f_darr2x(temp,P_num);
	f_darr2x(f,tmpykj);
	f_darr2x(z,tmpykj);

}

void readfile(double *bat){
	FILE *fp;
	//double *pena;
	//readfile1("penalty/48steps/JEPX.dat",FMStep,&pena);
	Pe=m_darr1(FMStep);
	/*for(int i=0;i<FMStep;i++){
		Pe[i]=30.0/1000;
	}*/
	/*for(int i=0;i<FMStep;i++){
		if(i>7&&i<22)Pe[i]=30.74/1000;
		else Pe[i]=9.48/1000;
	}*/
	for(int i=0;i<FMStep;i++){
		if(i>13&&i<46)Pe[i]=25.0/1000;
		else Pe[i]=11.0/1000;
	}
	readfile2("../../System/agentsys.dat");	
	//readfile1("demand/48steps/モデルケース4.dat",FMStep,&Lc_elec);	
	//readfile1("PVoutputs/48steps/モデルケース3.dat",FMStep,&PVout);
	readfile5("../../System/demand/48steps/TeXuan20Ben.dat", Load_cuvs, FMStep, &Lc_elecs);
	//readfile5("../../System/demand/48steps/特選20本.dat", Load_cuvs, FMStep, &Lc_elecs);
	readfile1("../../System/PVoutputs/48steps/QingLangZhiRi.dat", FMStep, &PVout);
	//readfile1("../../System/PVoutputs/48steps/晴れの日.dat", FMStep, &PVout);
	for(int i=0;i<FMStep;i++){
		PVout[i]*=Power*1000*TimeStep/FMStep;
	}
	readfile3("../../System/MIKAWA/f/inoue/market_date.dat",&P_num,&Min_p,&Max_p,&Unit_bid);
	Unit_p=(Max_p-Min_p)/P_num;
	if(Sta_d==0)*bat=0;
	else{
		if((fp=fopen("../../System/rest.csv","r"))==NULL){
			printf("%sが開けません","rest.csv");
			exit(0);
		}
		while(getc(fp)!='\n');
		fscanf(fp,"%lf",bat);
		fclose(fp);
	}
}

void readfile1(char *filename,int x,double **y){
	*y=m_darr1(x);
	FILE *fp;

	if((fp = fopen(filename,"r"))==NULL){
		printf("%sが開けません",filename);
		exit(0);
	}
	while(getc(fp)!='\n');
	for(int i=0;i<x;i++){
		fscanf(fp,"%lf",(*y)+i);
		while(getc(fp)!=',');
	}
	fclose(fp);
}

void readfile2(char *filename){
	Bat_eff=m_darr1(3);
	FILE *fp;
	
	if((fp = fopen(filename,"r"))==NULL){
		printf("%sが開けません",filename);
		exit(0);
	}
	while(getc(fp)!=':');
	fscanf(fp,"%lf",&(Batmax));
	while(getc(fp)!=':');
	fscanf(fp,"%lf",Bat_eff);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",Bat_eff+1);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",Bat_eff+2);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",&Power);
	fclose(fp);
	UnitBat=Batmax/(double)BatStep;
}


void readfile3(char *filename,int *u,double *v,double *w,double *x){
	FILE *fp;

	if((fp = fopen(filename,"r"))==NULL){
		printf("%sが開けません",filename);
		exit(0);
	}
	while(getc(fp)!=':');
	fscanf(fp,"%d",u);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",v);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",w);
	while(getc(fp)!=':');
	fscanf(fp,"%lf",x);
	fclose(fp);

}
void readfile4(int *p, HENV henv, HDBC hdbc,HSTMT hstmt){
	DWORD re_StandardNumber;
	_stprintf_s(SQLbuff,buffsize,TEXT("SELECT StandardNumber FROM AgreementPrice"));
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_StandardNumber, (SDWORD)sizeof(re_StandardNumber), NULL);
	while(1){
		RETCODE rc = SQLFetch(hstmt);
		if( rc == SQL_NO_DATA_FOUND ) break;
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
			printf("ERR\n");
			break;
		}
		*p = re_StandardNumber;
	}
	SQLFreeStmt(hstmt, SQL_DROP);
	SQLAllocStmt(hdbc, &hstmt);
}
void readfile5(char *filename,int x1,int x2,double ***y){
	*y=m_darr2(x1,x2);
	FILE *fp;

	if((fp = fopen(filename,"r"))==NULL){
		printf("%sが開けません",filename);
		exit(0);
	}
	for(int i=0;i<x1;i++){
		for(int j=0;j<x2;j++){
			fscanf(fp,"%lf",&((*y)[i][j]));
			while(getc(fp)!=',');
		}
	}
	fclose(fp);
}
void result1(char* filename,int**bid,int *t,int *n){
	FILE *fp;
	if((fp=fopen(filename,"w"))==NULL){
		printf("%sが開けません\n",filename);
		exit(0);
	}
	for(int i=0;i<*n+1;i++){
		fprintf(fp,"%d,",t[i]);
	}
	fprintf(fp,"\n");
	for(int p=0;p<P_num;p++){
		for(int i=0;i<*n+1;i++){
			fprintf(fp,"%d,",bid[i][p]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	*n=0;
}
void result2(char* filename,int *t,double *x){
	FILE *fp;
	
	if((fp=fopen(filename,"w"))==NULL){
		printf("%sが開けません\n",filename);
		exit(0);
	}
	for(int i=0;i<FMStep+1;i++){
		fprintf(fp,"%d,",t[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<FMStep+1;i++){
		fprintf(fp,"%lf,",x[i]);
	}
	fclose(fp);
}
void result3(char*filename,int*t,double*p,double*q,int *n){
	FILE *fp;
	if((fp=fopen(filename,"w"))==NULL){
		printf("%sが開けません\n",filename);
		exit(0);
	}
	for(int i=0;i<*n+1;i++){
		fprintf(fp,"%d,",t[i]);
	}
	fprintf(fp,"\n");	
	for(int i=0;i<*n+1;i++){
		fprintf(fp,"%lf,",p[i]);
	}
	fprintf(fp,"\n");	
	for(int i=0;i<*n+1;i++){
		fprintf(fp,"%lf,",q[i]);
	}
	fclose(fp);
	*n=0;
}
void result4(int d,int *tra_n,double *sho,double*co,int *rec_n,double*bat,double b){
	FILE *fp;
	char str[256];
	sprintf(str,"../../System/result/trade/%d.csv",d+Sta_d);
	result1(str,Trading,Tra_t,tra_n);
	sprintf(str,"../../System/result/short/%d.csv",d+Sta_d);
	result2(str,Act_t,sho);
	sprintf(str,"../../System/result/cost/%d.csv",d+Sta_d);
	result2(str,Act_t,co);
	sprintf(str,"../../System/result/agree/%d.csv",d+Sta_d);
	result3(str,Agree_t,Agree_p,Agree_q, rec_n);
	sprintf(str,"../../System/result/battery/%d.csv",d+Sta_d);
	result2(str,Act_t,bat);
	do fp=fopen("../../System/rest.csv","w");
	while(fp==NULL);
	fprintf(fp,"\n%lf,",b);
	fclose(fp);
}
void resulttest(int **em){
	FILE *fp;
	char str[256];
	sprintf(str,"EMアルゴリズム/test.dat");
	do fp = fopen(str,"w");
	while(fp==NULL);
	fprintf(fp,"\n");
	for(int i=0; i<TimeStep; i++){
		for(int j=0; j<ini; j++){
			fprintf(fp,"%d,",em[i][j]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
}
void result5(int d,double*** pro){
	int tmp=0;
	FILE *fp;
	char str[256];
	for(int i=0; i<TimeStep; i++){
		if(tmp < ykj[i])tmp = ykj[i];
	}
	if(tmp > ykjMAX) tmp=ykjMAX;
	sprintf(str,"../../System/result/EMAlgorithm/ContractPrice/%d.dat",d+Sta_d);
	do fp = fopen(str,"w");
	while(fp==NULL);
	fprintf(fp,"\n");
	for(int i=0; i<TimeStep; i++){
		for(int j=0; j<tmp; j++){
			fprintf(fp,"%.1lf,",EM_p[i][j]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	sprintf(str,"../../System/result/EMAlgorithm/TransitionProbability/%d.dat",d+Sta_d);
	do fp = fopen(str,"w");
	while(fp==NULL);
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){	
		for(int j=0;j<P_num;j++){
			for(int k=0;k<P_num;k++){
				fprintf(fp,"%lf,",pro[i][j][k]);
			}
			fprintf(fp,"\n");
		}
		fprintf(fp,"\n\n");
	}
	fclose(fp);
	sprintf(str,"../../System/result/EMAlgorithm/ykj/%d.dat",d+Sta_d);
	do fp = fopen(str,"w");
	while(fp==NULL);
	fprintf(fp,"\n");
	for(int i=0; i<TimeStep; i++){
		fprintf(fp,"%d\n",ykj[i]);
	}
	fprintf(fp,"\n");
	fclose(fp);
	sprintf(str,"../../System/result/EMAlgorithm/Value/%d.csv",d+Sta_d);
	result8(str,EM_gzai1,EM_gzai2,EM_ux1,EM_ux2,EM_uy1,EM_uy2,EM_sigma2,EM_sigmaxy1,EM_sigmaxy2);
}
void result8(char*filename,double*gzai1,double*gzai2,double*ux1,double*ux2,double*uy1,double*uy2,double*sigma2,double*sigmaxy1,double*sigmaxy2){
	FILE *fp;
	if((fp=fopen(filename,"w"))==NULL){
		printf("%sが開けません\n",filename);
		exit(0);
	}
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",gzai1[i]);
	}
	fprintf(fp,"\n");	
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",gzai2[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",ux1[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",ux2[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",uy1[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",uy2[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",sigma2[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",sigmaxy1[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<TimeStep;i++){
		fprintf(fp,"%lf,",sigmaxy2[i]);
	}
	fclose(fp);
}
int *m_iarr1(int i0){
	int *miar;
	miar=(int *) malloc(i0*sizeof(int));
	return(miar);
}

int **m_iarr2(int i0,int j0){
	int i;
	int **miar;
	miar=(int **) malloc(i0*sizeof(int*));
	for(i=0;i<i0;i++) miar[i]=m_iarr1(j0);
	return(miar);
}
int ***m_iarr3(int i0,int j0,int k0){
	int i;
	int ***miar;
	miar=(int ***) malloc(i0*sizeof(int**));
	for(i=0;i<i0;i++) miar[i]=m_iarr2(j0,k0);
	return(miar);
}
int ****m_iarr4(int i0,int j0,int k0,int l0)
{ int i;
  int ****miar;
  miar=(int ****) malloc(i0*sizeof(int***));
  for(i=0;i<i0;i++) miar[i]=m_iarr3(j0,k0,l0);
  return(miar);
}

double *m_darr1(int i0){
	double *mdar;
	mdar=(double *) malloc(i0*sizeof(double));
	return(mdar);
}

double **m_darr2(int i0,int i1){
	double **mdar;
	mdar=(double **) malloc(i0*sizeof(double*));
	for(int i=0;i<i0;i++) mdar[i]=m_darr1(i1);
	return(mdar);
}
double ***m_darr3(int i0,int j0,int k0){
	int i;
	double ***mdar;
	mdar=(double ***) malloc(i0*sizeof(double**));
	for(i=0;i<i0;i++) mdar[i]=m_darr2(j0,k0);
	return(mdar);
}
double ****m_darr4(int i0,int j0,int k0,int l0){
	int i;
	double ****mdar;
	mdar=(double ****) malloc(i0*sizeof(double***));
	for(i=0;i<i0;i++) mdar[i]=m_darr3(j0,k0,l0);
	return(mdar);
}
void f_iarr2(int **d,int i0,int j0){
	for(int i=0;i<i0;i++){
		free(*(d+i));
		d[i]=NULL;
	}
	free(d);
	d=NULL;
}
void f_darr2(double **d, int i0,int j0){
	int i;
	for(i=0;i<i0;i++) {free(*(d+i));d[i]=NULL;}
	free(d);
	d=NULL;
}

int *ex_iarr1(int *d,int i0){
	int*d2;
	d2=(int*)malloc(i0+1);
	memcpy(d2,d,sizeof(int)*P_num);
	free(d);
	return d2;
}
int **ex_iarr2(int **d,int i0){
	int **d2;
	d2=(int **) malloc((i0+1)*sizeof(int*));
	for(int i=0;i<(i0+1);i++){
		d2[i]=m_iarr1(P_num);
	}
	for(int j=0;j<i0;j++)memcpy(d2[j],d[j],sizeof(int)*P_num);
	f_iarr2(d,i0,P_num);
	return d2;
}

void f_iarr2x(int **d,int i0){
	for (int k=0 ;k<i0; k++){
		free(d[k]);
	}
	free(d);
}
void f_darr2x(double **d,int i0){
	for (int k=0 ;k<i0; k++){
		free(d[k]);
	}
	free(d);
}
bool Connect(TCHAR*server,TCHAR*uid,TCHAR*pwd,HENV*henv, HDBC*hdbc,HSTMT*hstmt){
	SQLAllocEnv(henv);
	SQLAllocConnect(*henv, hdbc);
	RETCODE rc = SQLConnect(*hdbc,(SQLTCHAR*)server,SQL_NTS,(SQLTCHAR*)uid,SQL_NTS,(SQLTCHAR*)pwd, SQL_NTS);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
		return false;
	}
	SQLAllocStmt(*hdbc, hstmt);
	return true;
}
void Close(HENV*henv, HDBC*hdbc,HSTMT*hstmt){
	SQLFreeStmt(hstmt, SQL_DROP);
	SQLDisconnect(*hdbc);
	SQLFreeConnect(*hdbc);
	SQLFreeEnv(*henv);
}

bool Execute(HENV*henv, HDBC*hdbc, HSTMT*hstmt,TCHAR*SQLStr){
	RETCODE rc = SQLExecDirect(*hstmt,(SQLTCHAR*)SQLStr, SQL_NTS);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)  {
	   return false;
	}
	return true;
}
void MakeTable(HENV henv, HDBC hdbc,HSTMT hstmt){
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE keep%d(OrderPrice INT,OrderQuantity INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE get%d(OrderPrice INT,OrderQuantity INT,OrderRank INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE keep_suc%d(KeepRank INT,KeepTime INT,BuyOrSell INT,OrderPrice INT,KeepQuantity INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE can%d(NumberOfCancel INT,Rank INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE rec%d(AgreeTime INT,BuyOrSell INT,OrderPrice INT,AgreePrice INT,AgreeQuantity INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE cancel_agree%d(AgreeTime INT,NumberOfCancel INT,CancelRank INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE enter%d(EnterRank INT,EnterPrice INT,EnterQuantity INT)"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
}
void DropTable(HENV henv, HDBC hdbc,HSTMT hstmt){
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE keep%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE get%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE keep_suc%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE can%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE rec%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE cancel_agree%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE enter%d"),ID);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
}
void Count(HENV henv, HDBC hdbc, HSTMT hstmt, int* line, TCHAR*SQLstr){
	DWORD re_SQLline;
	Execute(&henv,&hdbc,&hstmt,SQLstr);
	SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_SQLline, (SDWORD)sizeof(re_SQLline), NULL);
	while(1){
		RETCODE rc = SQLFetch(hstmt);
		if( rc == SQL_NO_DATA_FOUND ) break;
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
			printf("ERR\n");
			break;
		}
		*line=re_SQLline;
	}
	SQLFreeStmt(hstmt, SQL_DROP);
	SQLAllocStmt(hdbc, &hstmt);
}