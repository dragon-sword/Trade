#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <tchar.h>
#pragma comment(lib,"odbc32.lib")

int MN,Scale,D,Byo,H;
int M,N,kai;
int rank,m,day;
int **ta;
int *m_c,*m_m;
double start;
double Min_p,Unit_p,Unit_bid;
HENV   henv;
HDBC   hdbc;
HSTMT  hstmt;
int buffsize = 256;
TCHAR SQLbuff[256];
struct e_ord{
	int r;//順位
	int per;
	int pri;
	int qua;
	struct e_ord *next;
}n_keeps;
struct ranks{
	int r;//順位
	struct ranks *next;
}can_ran;
struct cans{
	struct ranks *rs;//キャンセルする注文のナンバー達
	struct cans *next;
}n_cans;
struct get{
	int per;
	int pri;
	int qua;
	int r;
	struct get *next;
}n_gets;
struct ord{
	int r;//順位
	int r2;
	int pri2;
	int per;
	int bid;
	int keep;
	int get;
	struct ord *next;
}*head0,*head1;

void con();
void initialize();
void ran(int*);
void alarm();
double s_clock(int);
void what_time();
void entrance();
void line(HENV, HDBC, HSTMT);
void suc_enter(struct e_ord*, HENV, HDBC, HSTMT);
void cancel(int*,int);
void trade();
void keep(int,int,ord*,ord*);
void keep_reset(ord*,get*);
void agree(int,int,int,ord*,ord*);
void rec0(int,int,int,int,int,int, HENV, HDBC, HSTMT);
void rec1(ord*,ord*,int,int, HENV, HDBC, HSTMT);
void rec2(int, HENV, HDBC, HSTMT);
void rec3();
void readfile(char*);
void read_N();
void result(char*,int**,int**);
int *m_iarr1(int);
int **m_iarr2(int,int);
double *m_darr1(int);
struct ord *m_ord_arr1(int);
bool Connect(TCHAR*,TCHAR*,TCHAR*,HENV*, HDBC*,HSTMT*);
void Close(HENV*, HDBC*,HSTMT*);
bool Execute(HENV*, HDBC*, HSTMT*,TCHAR*);
void MakeTable(HENV, HDBC, HSTMT);
void DropTable(HENV, HDBC, HSTMT);
void Count(HENV, HDBC, HSTMT, int*, TCHAR*);

void main(){
	printf("初期化中\n");
	con();
	initialize();
	_stprintf_s(SQLbuff,buffsize,TEXT("SQLtoMarket%d"),MN);
	if(!Connect(SQLbuff,TEXT("root"),TEXT("123456"),&henv,&hdbc,&hstmt)){
		printf("接続エラー\n");
	}
	MakeTable(henv,hdbc,hstmt);
	alarm();
	start=(double)clock();
	
	printf("市場動作開始\n");
	int kai=0;
	while(day<D){
		while(s_clock(day) <Byo){
			read_N();
			entrance();//入札の処理
			trade();//やり取り
		}
		day++;
		char str[256];
		sprintf(str,"m/%d日目.csv",day);
		result(str,&m_c,&m_m);
		if(day==D){
			break;//日を超えると終了日になる時
			Close(&henv,&hdbc,&hstmt); // 切断
			DropTable(henv,hdbc,hstmt);
		}
	}
}
void con(){
	MN=0;//市場の番号
	Scale=125                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       ;//時間の縮尺
	D=12;//日数
	Byo=86400;//一日の秒数
	H=12;//約定可能価格帯幅
	Min_p=0.005;
	Unit_p=0.002;
	Unit_bid=100;
}
void initialize(){
	rank = 0;
	day=0;
	m=110;
	readfile("../../System/MIKAWA/f/inoue/market_date.dat");
	ta=m_iarr2(M,2);
	m_c=m_iarr1(1);
	m_m=m_iarr1(1);
	for(int i0=0;i0<M;i0++)
		for(int i1=0;i1<2;i1++)ta[i0][i1]=0;
	head0=m_ord_arr1(M);
	head1=m_ord_arr1(M);
	for(int i=0;i<M;i++){
		head0[i].next=NULL;
		head1[i].next=NULL;
	}
}
void ran(int *agent){
	for(int i=0;i<N;i++){
		agent[i]=i;
	}
	for(int i=0;i<N;i++){
		int temp,target;
		srand((unsigned)clock());		
		target=rand()%N;;
		temp=agent[i];
		agent[i]=agent[target];
		agent[target]=temp;
	}
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
double s_clock(int da){
	double s_c;
	s_c=(((double)clock()-start)/1000)*Scale-da*Byo;
	return s_c;
}
void what_time(){
	int hour,min;
	double sec;
	hour=(int)s_clock(day)/3600;
	min=((int)s_clock(day)-hour*3600)/60;
	sec=s_clock(day)-hour*3600-min*60;
	printf("%d日目%d時%d分%.1lf秒\n",day,hour,min,sec);
}
void entrance(){
	line(henv,hdbc,hstmt);//新入札の並び替え(ランク付け)	
	//keep
	struct e_ord *v,*w;	
	while(1){
		int bs;
		struct ord *x=NULL,*y=NULL,*new_keep=NULL;

		v=n_keeps.next;
		w=&n_keeps;		
		if(v==NULL)break;
		if(v->qua>0){
			x=head0[v->pri].next;
			y=head0+(v->pri);
			bs=0;
		}
		else if(v->qua<0){
			x=head1[v->pri].next;
			y=head1+(v->pri);
			bs=1;
		}
		else {
			what_time();
			printf("何かがおかしい4\n");
		}

		while(x!=NULL){
			while((x->r)<(v->r)){
			y=x;
			x=x->next;
			if(x==NULL)break;
			}
		}		
		new_keep=(ord*)malloc(sizeof(ord));
		if(new_keep == NULL){
			puts("メモリ不足\n");
			return;
		}
		ta[v->pri][bs]+=v->qua;

		w->next=v->next;
		new_keep->r=v->r;
		new_keep->per=v->per;
		new_keep->bid=v->qua;
		new_keep->keep=0;
		new_keep->get=0;
		new_keep->next=x;
		y->next=new_keep;
		suc_enter(v,henv,hdbc,hstmt);
		free(v);
		rec3();
	}
	struct get *g0,*g1;
	//本注文//
	while(1){
		int bs;
		struct ord *x=NULL,*y=NULL,*part0=NULL,*part1=NULL;

		g0=n_gets.next;
		g1=&n_gets;
		if(g0==NULL)break;
		else if(g0->qua>0){
			x=head0[g0->pri].next;
			y=head0+(g0->pri);
			bs=0;
		}
		else if(g0->qua<0){
			x=head1[g0->pri].next;
			y=head1+(g0->pri);
			bs=1;
		}
		else {
			printf("何かがおかしい4\n");
			what_time();
		}
		while(x!=NULL){
			if((x->r)!=(g0->r)){
				y=x;
				x=x->next;				
			}
			else break;
		}
		if(x==NULL){
			what_time();
			printf("%dの%lf円注文はキャンセル済みです\n",g0->per,(Min_p+g0->pri*Unit_p)*1000);
			break;
		}
		
		if(g0->qua>0){
			part0=head1[x->pri2].next;
			part1=head1+(x->pri2);
		}
		else if(g0->qua<0){
			part0=head0[x->pri2].next;
			part1=head0+(x->pri2);
		}
		while(part0!=NULL){
			if((part0->r)!=(x->r2)){
				part1=part0;
				part0=part0->next;
			}
			else break;
		}

		if(part0==NULL){//相手がちゅうもんを取り下げてた時。keepをbidに戻す
			keep_reset(x,g0);
			printf("%dの%lf円注文は相手が取り下げてました\n",g0->per,(Min_p+g0->pri*Unit_p)*1000);
			what_time();
		}	
		else{
			x->keep-=g0->qua;
			x->get+=g0->qua;
			if(part0->get!=0){//相手もkeepを取りに来ている時
				int q;
				q=abs(g0->qua);					
				if(bs==0)agree(g0->pri,x->pri2,q,x,part0);
				else agree(x->pri2,g0->pri,q,part0,x);
				kai++;
				m_c=(int*)realloc(m_c,sizeof(int)*(kai+1));
				m_m=(int*)realloc(m_m,sizeof(int)*(kai+1));
				m_c[kai]=(int)(s_clock(day));
				m_m[kai]=m;
				//繰り上げ//
				if(part0->bid!=0||part0->keep!=0||part0->get!=0)printf("%dの%.1lf円の注文はbidが%.1lfWh,keepが%.1lfWh,getが%.1lfWh残っています\n",part0->per,(Min_p+x->pri2*Unit_p)*1000,part0->bid*Unit_bid,part0->keep*Unit_bid,part0->get*Unit_bid);
				if(x->bid!=0||x->keep!=0||x->get!=0) printf("%dの%.1lf円の注文はbidが%.1lfWh,keepが%.1lfWh,getが%.1lfWh残っています\n",x->per,(Min_p+part0->pri2*Unit_p)*1000,x->bid*Unit_bid,x->keep*Unit_bid,x->get*Unit_bid);
				if(part0->bid==0&&part0->keep==0&&part0->get==0){
					part1->next=part0->next;
					what_time();
					printf("%dの%.1lf円の注文は全て約定されました。\n",part0->per,(Min_p+x->pri2*Unit_p)*1000);
					free(part0);
				}
				if(x->bid==0&&x->keep==0&&x->get==0){
					y->next=x->next;
					what_time();
					printf("%dの%.1lf円の注文は全て約定されました。\n",x->per,(Min_p+g0->pri*Unit_p)*1000);
					free(x);
				}					
				rec3();
			}
			else if(part0->keep==0){//相手がキャンセルしてしまっている時
				keep_reset(x,g0);
				printf("%dの%lf円注文は相手が取り下げてました\n",g0->per,(Min_p+g0->pri*Unit_p)*1000);
				what_time();
			}
		}
		g1->next=g0->next;
		free(g0);
		rec3();
	}
}
void line(HENV henv, HDBC hdbc, HSTMT hstmt){//当期の新入札を順に並べて、それを取引の順番待ちに組み込む
	/*入札データの読み込み*/
	int *agent;
	n_keeps.next=NULL;
	n_gets.next=NULL;
	n_cans.next=NULL;
	agent=m_iarr1(N);
	DWORD re_OrderPrice,re_OrderQuantity,re_OrderRank,re_NumberOfCancel,re_Rank;
	int SQLline;
	ran(agent);
	for(int i=0;i<N;i++){
		_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM keep%d"),agent[i]);
		Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

		if( SQLline != 0 ){
			_stprintf_s(SQLbuff,buffsize,TEXT("SELECT OrderPrice,OrderQuantity FROM keep%d"),agent[i]);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);	
			SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_OrderPrice, (SDWORD)sizeof(re_OrderPrice), NULL);
			SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_OrderQuantity, (SDWORD)sizeof(re_OrderQuantity), NULL);
			while(1){
				int p=0,q=0;
				struct e_ord *x,*y,*new_keep;
				new_keep=(struct e_ord*)malloc(sizeof(struct e_ord));
				
				x = n_keeps.next;
				y = &n_keeps;

				RETCODE rc = SQLFetch(hstmt);
				if( rc == SQL_NO_DATA_FOUND ) break;
				if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
					printf("ERR\n");
					break;
				}
				p = re_OrderPrice;
				q = re_OrderQuantity;

				new_keep->r=rank;
				rank++;
				new_keep->per=agent[i];
				new_keep->pri=p;
				new_keep->qua=q;
				new_keep->next=NULL;
				while(x!=NULL){
					y=x;
					x= x->next;
				}
				y->next = new_keep;
			}
		}
		SQLFreeStmt(hstmt, SQL_DROP);
		SQLAllocStmt(hdbc, &hstmt);
		_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE keep%d"),agent[i]);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
	}
					
	for(int i=0;i<N;i++){
		_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM get%d"),agent[i]);
		Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

		if( SQLline != 0 ){
			_stprintf_s(SQLbuff,buffsize,TEXT("SELECT OrderPrice,OrderQuantity,OrderRank FROM get%d"),agent[i]);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);	
			SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_OrderPrice, (SDWORD)sizeof(re_OrderPrice), NULL);
			SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_OrderQuantity, (SDWORD)sizeof(re_OrderQuantity), NULL);
			SQLBindCol(hstmt, 3, SQL_C_SLONG, &re_OrderRank, (SDWORD)sizeof(re_OrderRank), NULL);
			while(1){
				int p,q,r;
				struct get *x,*y,*new_get;
				new_get=(struct get*)malloc(sizeof(struct get));
				
				x = n_gets.next;
				y = &n_gets;

				RETCODE rc = SQLFetch(hstmt);
				if( rc == SQL_NO_DATA_FOUND ) break;
				if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
					printf("ERR\n");
					break;
				}
				p = re_OrderPrice;
				q = re_OrderQuantity;
				r = re_OrderRank;
				new_get->per=agent[i];
				new_get->pri=p;
				new_get->qua=q;				
				new_get->r=r;
				new_get->next=NULL;
				while(x!=NULL){
					y=x;
					x= x->next;
				}
				y->next = new_get;
			}
		}
		SQLFreeStmt(hstmt, SQL_DROP);
		SQLAllocStmt(hdbc, &hstmt);
		_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE get%d"),agent[i]);
		Execute(&henv,&hdbc,&hstmt,SQLbuff);
	}

	for(int i0=0;i0<N;i0++){
		_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM can%d"),agent[i0]);
		Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

		if( SQLline != 0 ){
			int num,*ranks;
			_stprintf_s(SQLbuff,buffsize,TEXT("SELECT NumberOfCancel,Rank FROM can%d"),agent[i0]);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);	
			SQLBindCol(hstmt, 1, SQL_C_SLONG, &re_NumberOfCancel, (SDWORD)sizeof(re_NumberOfCancel), NULL);
			SQLBindCol(hstmt, 2, SQL_C_SLONG, &re_Rank, (SDWORD)sizeof(re_Rank), NULL);
			RETCODE rc = SQLFetch(hstmt);
			num = re_NumberOfCancel;
			ranks=m_iarr1(num);
			ranks[0] = re_Rank;
			int i2=1;
			while(1){
				RETCODE rc = SQLFetch(hstmt);
				if( rc == SQL_NO_DATA_FOUND ) break;
				if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
					printf("ERR\n");
					break;
				}
				ranks[i2] = re_Rank;
				i2++;
			}
			SQLFreeStmt(hstmt, SQL_DROP);
			SQLAllocStmt(hdbc, &hstmt);
			_stprintf_s(SQLbuff,buffsize,TEXT("TRUNCATE TABLE can%d"),agent[i0]);
			Execute(&henv,&hdbc,&hstmt,SQLbuff);
			printf("%dのキャンセルが入りました。\n",agent[i0]);
			cancel(ranks,num);

			_stprintf_s(SQLbuff,1024,TEXT("SELECT COUNT(*) FROM cancel_agree%d"),agent[i0]);
			Count(henv, hdbc, hstmt, &SQLline, SQLbuff);

			if( SQLline != 0 ) printf("変,変だよ・・・");
			else{
				int time=(int)s_clock(day);
				SQLSetConnectAttr(hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
				for(int i4=0;i4<num;i4++){
					_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO cancel_agree%d(AgreeTime,NumberOfCancel,CancelRank) VALUES(%d,%d,%d)"),agent[i0],time,num,ranks[i4]);
					Execute(&henv,&hdbc,&hstmt,SQLbuff);
				}
				SQLTransact( henv, hdbc, SQL_COMMIT );
				SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
				what_time();
				printf("キャンセル成功！\n");
				break;
			}
			free(ranks);
		}
	}
	free(agent);
}
void suc_enter(struct e_ord *neword, HENV henv, HDBC hdbc, HSTMT hstmt){
	int time=(int)s_clock(day);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO enter%d(EnterRank,EnterPrice,EnterQuantity) VALUES(%d,%d,%d)"),neword->per,neword->r,neword->pri,neword->qua);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	what_time();
	printf("%dの%.1lf円の注文が%.1lfWh入りました。\n",neword->per,(Min_p+neword->pri*Unit_p)*1000,neword->qua*Unit_bid);
}

void cancel(int*ranks,int num){
	for(int i=0;i<num;i++){
		struct ord *x,*y;
		for(int j=0;j<M;j++){
			x=head0[j].next;
			y=head0+j;
			while(x!=NULL){
				if((x->r)==ranks[i])break;
				else{
					y=x;
					x=x->next;
					if(x==NULL)break;
				}
			}
			if(x!=NULL){
				ta[j][0]-=x->bid;
				y->next=x->next;
				free(x);//キャンセルされた！！
				break;
			}
			//価格jの売りには注文がなかった時//
			x=head1[j].next;
			y=head1+j;
			while(x!=NULL){
				if((x->r)==ranks[i])break;
				else{
					y=x;
					x=x->next;
					if(x==NULL)break;
				}
			}
			if(x!=NULL){
				ta[j][1]-=x->bid;
				y->next=x->next;
				free(x);//キャンセルされた！！
				break;
			}
			//価格jには注文がなかった//
		}
	}
}
void trade(){
	for(int i=0;i<M;i++){
		if(ta[i][0]!=0){//ある売り注文について
			for(int j=M-1;j>=i;j--){
				if(ta[j][1]!=0){//対当する買い注文がある時
					while(ta[j][1]!=0 &&ta[i][0]!=0){
						struct ord *b0,*b1,*s0,*s1;
						//一番早い注文について//						
						s0=head0+i;
						s1=s0->next;
						b0=head1+j;
						b1=b0->next;
						while(s1->bid==0){
							s0=s1;
							s1=s1->next;
						}
						while(b1->bid==0){
							b0=b1;
							b1=b1->next;
						}
						keep(i,j,s1,b1);
						kai++;
						m_c=(int*)realloc(m_c,sizeof(int)*(kai+1));
						m_m=(int*)realloc(m_m,sizeof(int)*(kai+1));
						m_c[kai]=(int)(s_clock(day));
						m_m[kai]=m;
						//繰り上げ//
						if(s1->bid==0&&s1->keep==0&&s1->get==0){
							s0->next=s1->next;
							what_time();
							printf("%dの%.1lfの注文が約定しきりました\n",s1->per,(Min_p+i*Unit_p)*1000);
							free(s1);
						}
						if(b1->bid==0&&b1->keep==0&&b1->get==0){
							b0->next=b1->next;
							what_time();
							printf("%dの%.1lfの注文が約定しきりました\n",b1->per,(Min_p+j*Unit_p)*1000);
							free(b1);
						}
						rec3();
					}
				}
			}
		}
	}
}
void keep(int sp,int bp,ord *s,ord *b){
	int q;//qは正
	if(s->bid>=-b->bid)q=-b->bid;
	else q=s->bid;	
	rec1(s,b,sp,bp,henv,hdbc,hstmt);
	s->keep=s->bid;
	ta[sp][0]-=s->bid;
	s->bid=0;
	ta[bp][1]-=b->bid;
	b->keep=b->bid;
	b->bid=0;
	s->r2=b->r;
	s->pri2=bp;
	b->r2=s->r;
	b->pri2=sp;
	/*if((s->r)>(b->r)){
		if((s->qua)>-(b->qua)){//約定量-(b->qua)
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
		else if((s->qua)==-(b->qua)){//約定量s->qua
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
		else if((s->qua)<-(b->qua)){//約定量s->qua
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
	}
	else{
		if((s->qua)>-(b->qua)){//約定量-(b->qua)
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
		else if((s->qua)==-(b->qua)){//約定量s->qua
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
		else if((s->qua)<-(b->qua)){//約定量s->qua
			ta[sp][0]-=s->qua;
			ta[bp][1]-=b->qua;
			rec1(s,b,sp,bp,henv,hdbc,hstmt);
			s->k=1;
			s->r2=b->r;
			s->pri2=bp;
			b->k=1;
			b->r2=s->r;
			b->pri2=sp;
		}
	}*/
}

void agree(int sp,int bp,int q,ord *s,ord *b){
	if((s->r)>(b->r)){
		rec0(q,bp,s->per,b->per,sp,bp,henv,hdbc,hstmt);
		what_time();
		printf("%.1lf円で%dと%dが約定しました1\n",(Min_p+bp*Unit_p)*1000,s->per,b->per);
		s->get-=q;
		if(s->get!=0||s->keep!=0){//getの見込みがなくなったらbidに戻す
			s->bid+=s->get+s->keep;
			ta[sp][0]+=s->get+s->keep;
			s->keep=0;
			s->get=0;
		}
		b->get+=q;
		if(b->get!=0||b->keep!=0){
			b->bid+=b->get+b->keep;
			ta[bp][1]+=b->get+b->keep;
			b->keep=0;
			b->get=0;
		}
		m=bp;
		rec2(m,henv,hdbc,hstmt);
	}
	else{
		rec0(q,sp,s->per,b->per,sp,bp,henv,hdbc,hstmt);
		what_time();
		printf("%.1lfで%dと%dが約定しました2\n",(Min_p+sp*Unit_p)*1000,s->per,b->per);			
		s->get-=q;
		if(s->get!=0||s->keep!=0){//getの見込みがなくなったらbidに戻す
			s->bid+=s->get+s->keep;
			ta[sp][0]+=s->get+s->keep;
			s->keep=0;
			s->get=0;
		}
		b->get+=q;
		if(b->get!=0||b->keep!=0){
			b->bid+=b->get+b->keep;
			ta[bp][1]+=b->get+b->keep;
			b->keep=0;
			b->get=0;
		}		
		m=sp;
		rec2(m,henv,hdbc,hstmt);
	}
}
void keep_reset(struct ord *x,struct get *y){
	int bs;
	printf("%lf円、%lfWhのkeepをresetしました\n",(Min_p+y->pri*Unit_p)*1000,x->keep*Unit_bid);
	x->bid+=x->keep;
	if(x->keep>0)bs=0;
	else bs=1;
	ta[y->pri][bs]+=x->keep;
	x->keep=0;
}
void rec0(int q,int p,int s_per,int b_per,int sp,int bp, HENV henv, HDBC hdbc, HSTMT hstmt){
	int time=(int)s_clock(day);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO rec%d(AgreeTime,BuyOrSell,OrderPrice,AgreePrice,AgreeQuantity) VALUES(%d,%d,%d,%d,%d)"),s_per,time,0,sp,p,q);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	what_time();
	printf("%.1lf円/kWhで%.1lfWhの売り注文を約定\n",(Min_p+sp*Unit_p)*1000,q*Unit_bid);	

	time=(int)s_clock(day);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO rec%d(AgreeTime,BuyOrSell,OrderPrice,AgreePrice,AgreeQuantity) VALUES(%d,%d,%d,%d,%d)"),b_per,time,1,bp,p,-q);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	what_time();
	printf("%.1lf円/kWhで%.1lfWhの売り注文を約定\n",(Min_p+sp*Unit_p)*1000,q*Unit_bid);	
}
void rec1(ord *s,ord *b,int sp,int bp, HENV henv, HDBC hdbc, HSTMT hstmt){
	int qua;
	if(s->bid>-(b->bid))qua=-(b->bid);
	else qua=s->bid;

	int time=(int)s_clock(day);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO keep_suc%d(KeepRank,KeepTime,BuyOrSell,OrderPrice,KeepQuantity) VALUES(%d,%d,%d,%d,%d)"),s->per,s->r,time,0,sp,qua);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	what_time();
	printf("%.1lf円/kWhで%.1lfWhの売り注文をkeep\n",(Min_p+sp*Unit_p)*1000,qua*Unit_bid);

	time=(int)s_clock(day);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO keep_suc%d(KeepRank,KeepTime,BuyOrSell,OrderPrice,KeepQuantity) VALUES(%d,%d,%d,%d,%d)"),b->per,b->r,time,1,bp,-qua);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
	what_time();
	printf("%.1lf円/kWhで%.1lfWhの売り注文をkeep\n",(Min_p+sp*Unit_p)*1000,qua*Unit_bid);
}
void rec2(int mp, HENV henv, HDBC hdbc, HSTMT hstmt){
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("UPDATE AgreementPrice SET StandardNumber = %d"),mp);
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
}


void rec3(){
	FILE *fp;
	char str[256];
	for(int i=0;i<M;i++){
		if(ta[i][0]!=0){//ある価格に売り注文がある時
			sprintf(str,"../../System/MIKAWA/f/inoue/market%d/min_sell.dat",MN);
			do fp = fopen(str,"w");
			while(fp==NULL);
			fprintf(fp,"minimum price of order for sell:%d\n",i);
			fprintf(fp,"quantity of minimum price of order for sell:%d",ta[i][0]);
			fclose(fp);
			break;
		}
		if(i==M-1){
			if(ta[i][0]==0){
				sprintf(str,"../../System/MIKAWA/f/inoue/market%d/min_sell.dat",MN);
				do fp = fopen(str,"w");
				while(fp==NULL);
				fprintf(fp,"minimum price of order for sell:%d\n",M);
				fprintf(fp,"quantity of minimum price of order for sell:%d",ta[i][0]);
				fclose(fp);
			}
		}
	}
	for(int i=M-1;i>=0;i--){
		if(ta[i][1]!=0){//ある価格に売り注文がある時
			sprintf(str,"../../System/MIKAWA/f/inoue/market%d/max_buy.dat",MN);
			do fp = fopen(str,"w");
			while(fp==NULL);
			fprintf(fp,"maximum price of order for buy:%d\n",i);
			fprintf(fp,"quantity of maximum price of order for buy:%d",ta[i][1]);
			fclose(fp);
			break;
		}
		if(i==0){
			if(ta[i][0]==0){
				sprintf(str,"../../System/MIKAWA/f/inoue/market%d/max_buy.dat",MN);
				do fp = fopen(str,"w");
				while(fp==NULL);
				fprintf(fp,"maximum price of order for buy:%d\n",0);
				fprintf(fp,"quantity of maximum price of order for buy:%d",ta[i][1]);
				fclose(fp);
			}
		}
	}
}
void readfile(char *filename){
	FILE *fp;
	do fp = fopen(filename,"r");
	while(fp==NULL);
	while(getc(fp)!=':');
	fscanf(fp,"%d",&M);
	fclose(fp);
}

void read_N(){
	int i=0;
	FILE *fp;
	char str[256];
	while(1){
		sprintf(str,"../../System/MIKAWA/f/inoue/market%d/%d/name.dat",MN,i);
		if((fp = fopen(str,"r"))==NULL){
			N=i;
			break;
		}
		else fclose(fp);
		i++;
	}
}
void result(char* filename,int **m_c,int **m_m){
	FILE *fp;
	do fp = fopen(filename,"w");
	while(fp==NULL);
	for(int i=0;i<kai;i++){
		fprintf(fp,"%d,",(*m_c)[i]);
	}
	fprintf(fp,"\n");
	for(int i=0;i<kai;i++){
		fprintf(fp,"%d,",(*m_m)[i]);
	}
	fclose(fp);
	kai=0;
	free(*m_c);
	free(*m_m);
	*m_c=m_iarr1(1);
	*m_m=m_iarr1(1);
}
double *m_darr1(int i0){
	double *mdar;
	mdar=(double *)malloc(i0*sizeof(double));
	if (mdar == NULL)printf("確保失敗");
	//if (mdar == NULL)printf("メモリ確保失敗");
	return(mdar);
}
int *m_iarr1(int i0){
	int *miar;
	miar=(int *)malloc(i0*sizeof(int));
	return(miar);
}

int **m_iarr2(int i0,int i1){
	int **miar;
	miar=(int **)malloc(i0*sizeof(int*));
	for(int i=0;i<i0;i++)miar[i]=m_iarr1(i1);
	return(miar);
}

struct ord *m_ord_arr1(int i0){
	struct ord *mdar;
	mdar=(struct ord *)malloc(i0*sizeof(struct ord));
	return(mdar);
}
bool Connect(TCHAR*server,TCHAR*uid,TCHAR*pwd,HENV*henv, HDBC*hdbc,HSTMT*hstmt){
	RETCODE result = 0;
	result = SQLAllocEnv(henv);
	result = SQLAllocConnect(*henv, hdbc);
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
void MakeTable(HENV henv, HDBC hdbc, HSTMT hstmt){
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE AgreementPrice"));
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	_stprintf_s(SQLbuff,buffsize,TEXT("CREATE TABLE AgreementPrice(StandardNumber INT)"));
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	_stprintf_s(SQLbuff,buffsize,TEXT("INSERT INTO AgreementPrice(StandardNumber) VALUES(5)"));
	Execute(&henv,&hdbc,&hstmt,SQLbuff);
	SQLTransact( henv, hdbc, SQL_COMMIT );
	SQLSetConnectAttr( hdbc, SQL_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_ON, 0);
}
void DropTable(HENV henv, HDBC hdbc, HSTMT hstmt){
	_stprintf_s(SQLbuff,buffsize,TEXT("DROP TABLE AgreementPrice"));
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