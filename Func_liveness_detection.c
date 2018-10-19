#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

/* 函数声明 */
bool liveness_detection(char *DeepFile, int rec_face[4]);  
// Eg:rec_face = {157 ,66 ,172 ,198 }, 行取66：66+198,列取157：157+172

int main()
{
	 /* 局部变量定义 */
	char *DeepFile = "/home/zhoujie/liveness detection/raw文件/non-face/18.raw";
    int rec_face[4] = {85 ,162 ,172 ,181};
	bool IS_FACE;

	/* 调用函数判断是否为活体 */
	IS_FACE = liveness_detection( DeepFile, rec_face);
	printf("%d\n", IS_FACE);

}




/* 函数判断是否为活体 */	
bool liveness_detection(char *DeepFile, int rec_face[4])
{
	const int IMG_HEIGHT =  345;
    const int IMG_WIDTH =  400;
    const int ITER = 10000; // 随机取点次数
    const float PLANE_OR_NOT = 0.1; // 判断是否为平面的分界线
	const int SIGMA = 1;
    typedef unsigned short UNIT16;
	
	// 从.raw读取二进制16位数据到MatDATA
	UNIT16 MatDATA[IMG_HEIGHT*IMG_WIDTH];
	FILE *fp = NULL;
	fp = fopen( DeepFile, "rb" );
    fread(MatDATA,sizeof(UNIT16),IMG_HEIGHT*IMG_WIDTH,fp);
	fclose(fp);
	// int length = sizeof(MatDATA) / sizeof(UNIT16); 
	// printf("数组的长度为: %d\n",length); //length 应为IMG_HEIGHT*IMG_WIDTH

	// DeepDATA三行分别为深度图行数，列数，深度信息
	// int DeepDATA[3][IMG_HEIGHT*IMG_WIDTH];
	int n = 0;
	int i,j;
	// for(i=1;i< IMG_HEIGHT+1 ;i++)
	// 	{
	// 	    for(j=1;j< IMG_WIDTH+1 ;j++) 
	// 	    { 
	// 			DeepDATA[0][n] = i;
	// 			DeepDATA[1][n] = j;
	// 			DeepDATA[2][n] = MatDATA[n];
	// 			n += 1;
	// 		} 
	// 	} 
	// int test1 = 110194 ;
	// printf("%d,%d,%d\n",DeepDATA[0][test1],DeepDATA[1][test1],DeepDATA[2][test1]);	
	
	// FaceDATA为深度图DeepDATA裁剪后且去除零深度信息后的人脸部分
	int COL = rec_face[0],ROW = rec_face[1],FACE_WIDTH = rec_face[2],FACE_HEIGHT = rec_face[3]; //位置信息
	// txt :157 66 172 198 , 取行66：66+198,列取157：157+172
	int FaceDATA[3][FACE_HEIGHT*FACE_WIDTH];
	int faceno0_num = FACE_HEIGHT*FACE_WIDTH -1; 
	n = 0;
	for(i = 1;i< FACE_HEIGHT+1;i++)
		{
		    for(j= 1;j< FACE_WIDTH+1;j++) 
		    { 
				if (MatDATA[IMG_WIDTH*(ROW+i-2)+COL+j-2] == 0)
				{
					faceno0_num -= 1; // 非零深度点个数为 faceno0_num+1
					continue;
				}
				FaceDATA[0][n] = i;
				FaceDATA[1][n] = j; 
				FaceDATA[2][n] = MatDATA[IMG_WIDTH*(ROW+i-2)+COL+j-2];
				n += 1;
			} 
		} 
	// int test = 6804;  
	// printf("%d,%d,%d,%d\n",test,FaceDATA[0][test],FaceDATA[1][test],FaceDATA[2][test]);
    int pretotal = 0;  // 符合拟合模型的数据的个数
	int x[3],y[3],z[3];  // 随机取三个点 
	srand((unsigned)time(NULL));
	float a,b,c;  // 拟合平面方程 z=ax+by+c
	// float besta,bestb,bestc;  // 最佳参数
	int rand_num[3];
	float check,distance;
	int total = 0;
	for(i = 0; i < ITER; i++)
	{
		do{
			rand_num[0] = rand()%faceno0_num; 
			rand_num[1] = rand()%faceno0_num; 
			rand_num[2] = rand()%faceno0_num; 
		}while(rand_num[0] == rand_num[1] || rand_num[0] == rand_num[2] || rand_num[1] == rand_num[2]);
		for(n = 0; n < 3; n++ )
		{
			x[n] = FaceDATA[0][rand_num[n]];
			y[n] = FaceDATA[1][rand_num[n]];
			z[n] = FaceDATA[2][rand_num[n]];
			// printf("%d,%d,%d,%d\n", x[n],y[n],z[n],n);
		}
		check = (x[0]-x[1])*(y[0]-y[2]) - (x[0]-x[2])*(y[0]-y[1]);
		if ( check == 0)  // 防止提示浮点数例外 (核心已转储)
		{
			i -= 1;
			continue;
		}
		a = ( (z[0]-z[1])*(y[0]-y[2]) - (z[0]-z[2])*(y[0]-y[1]) )/( (x[0]-x[1])*(y[0]-y[2]) - (x[0]-x[2])*(y[0]-y[1]) );
        if (y[0] == y[2])  // 防止提示浮点数例外 (核心已转储)
		{
			i -= 1;
			continue;
		}
		b = ((z[0] - z[2]) - a * (x[0] - x[2]))/(y[0]-y[2]);
        c = z[0]- a * x[0] - b * y[0];
		// printf("%f,%f,%f\n",a,b,c);
		total = 0;
		for(n = 0; n < faceno0_num +1 ; n++ )
		{
			distance = fabs(a*FaceDATA[0][n] + b*FaceDATA[1][n] - 1*FaceDATA[2][n] + c*1);
			if (distance < SIGMA)
			{
				total +=1;
			}
		}
		// printf("%d,%f,%d\n",i,distance,total);
		if (total > pretotal)  // 找到符合拟合平面数据最多的拟合平面
        {
			pretotal=total;
			// besta = a;
			// bestb = b;
			// bestc = c;
		}
	}
	float pretotal_ary = pretotal *1.0/ faceno0_num ;
	// printf("%d,%f\n", pretotal,pretotal_ary);
	bool IS_FACE;

    if (pretotal_ary>PLANE_OR_NOT)
	{
		IS_FACE =  true;
		// printf("是人脸");
	}
	else
	{
		IS_FACE = false;
		// printf("不是人脸");
	}
	// printf("%d\n", IS_FACE);
	return  IS_FACE;
} 

