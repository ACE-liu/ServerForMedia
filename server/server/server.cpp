// server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "ffmpegHeader.h"
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include "Server_Sock.h"
#include <time.h>
#pragma   comment(lib, "Ws2_32.lib ") 

#define USER_COUNT 1
//#define UN_CONVERSION 1
//#define CONVERSION 0

typedef struct UserMessage
{
	bool has_get_rtmp;
	char *in_rtsp_address;
	char *out_rtmp_address;
};
typedef struct data1
{
	int64_t len;
	char *data;
	char a;
};
void Init();
int getRtmpURLByInput(const char*, const char *);
DWORD WINAPI exchangeThread(LPVOID);
void show_dshow_device();
int initMessageList();
int getRtmp();


UserMessage MessageList[USER_COUNT];

void printCurTime()
{
	//data ldata;
	//printf("%d\n", sizeof(data1));
	time_t timep;
	time(&timep);
	char buf[64];
	int a = sizeof(buf);
	strftime(buf, a, "%Y-%m-%d %H:%M:%S", localtime(&timep));
	printf("%s", timep);
}

int main(int argc, _TCHAR* argv[])
{
	printf("%d\n", sizeof(data1));
	AVFormatContext *pFormatCtx = NULL;
	Init();
	initMessageList();
	getRtmp();
	printf("start server...\n");
	system("pause");
	return 0;
}


int initMessageList()
{
	MessageList[0].has_get_rtmp = false;
	//MessageList[0].in_rtsp_address = "rtsp://admin:123456@120.76.211.226:40901/h264/ch1/sub/av_stream";s
	MessageList[0].in_rtsp_address = "rtsp://admin:123456@120.76.211.226:40901/h264/ch1/sub/av_stream";
	MessageList[0].out_rtmp_address = "rtmp://127.0.0.1:1935/myapp/40901";
	return 0;
}
int getRtmp()
{
	for (size_t i = 0; i < USER_COUNT; i++)
	{
		//CreateThreadpool
		if (!MessageList[i].has_get_rtmp)
		{
			HANDLE subHandle = CreateThread(NULL, 0, exchangeThread, (LPVOID)&MessageList[i], 0, NULL);
			if (subHandle == NULL)
			{
				printf("failed to create a new thread...");
			}
			CloseHandle(subHandle);
		}
	}
	return 0;
}

void show_dshow_device() {
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("Device Info=============\n");
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
	printf("========================\n");
}
DWORD WINAPI exchangeThread(LPVOID lpparater)
{
	//Init();
	UserMessage message = *(UserMessage*)lpparater;
	char *infilename = message.in_rtsp_address;
	char *outfilename = message.out_rtmp_address;
	getRtmpURLByInput(infilename, outfilename);
	return 0;
}
void Init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	//av_log_set_level(AV_LOG_ERROR);
}
int64_t lastpts = -1;
int64_t lastdura = -1;

int getUserCameraStream(const char *out_filename)
{
	if (!out_filename)
	{
		fprintf(stderr, "out_filename can not be null...");
		return -1;
	}
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;
	AVInputFormat *ifmt = NULL;
	AVCodec *pCodec=NULL;
	AVCodecContext *pCodecCtx = NULL;
	char in_filename[30];
	char *cameraName = "USB2.0 PC CAMERA";
	char*audioName = "麦克风 (USB2.0 MIC)";
	sprintf(in_filename, "video=%s", cameraName);
	int ret,i;
	int videoIndex = -1;
	AVPacket pkt;
	int frame_index = 0;
	ifmt = av_find_input_format("dshow");


	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, ifmt, NULL)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret=avformat_find_stream_info(ifmt_ctx,NULL))<0)
	{
		printf("find stream info failed...");
		goto end;
	}
	for  (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		if (ifmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			break;
		}
	}
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);

	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	ofmt = ofmt_ctx->oformat;
	//printf("%s : streams: %d\n", in_filename, ifmt_ctx->nb_streams);
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format------------------
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!pCodec) {
		printf("Can not find encoder! (没有找到合适的编码器！)\n");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = ifmt_ctx->streams[videoIndex]->codec->width;
	pCodecCtx->height = ifmt_ctx->streams[videoIndex]->codec->height;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 250;
	/* Some formats,for example,flv, want stream headers to be separate. */
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	//H264 codec param  
	//pCodecCtx->me_range = 16;  
	//pCodecCtx->max_qdiff = 4;  
	//pCodecCtx->qcompress = 0.6;  
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;
	//Optional Param  
	pCodecCtx->max_b_frames = 3;
	// Set H264 preset and tune  
	AVDictionary *param = 0;
	av_dict_set(&param, "preset", "fast", 0);
	av_dict_set(&param, "tune", "zerolatency", 0);

	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
		printf("Failed to open encoder! (编码器打开失败！)\n");
		return -1;
	}

	//Add a new stream to output,should be called by the user before avformat_write_header() for muxing  
	AVStream* video_st = avformat_new_stream(ofmt_ctx, pCodec);
	if (video_st == NULL) {
		return -1;
	}
	video_st->time_base.num = 1;
	video_st->time_base.den = 25;
	video_st->codec = pCodecCtx;
	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}
	int64_t start_time;
	start_time = av_gettime();
	bool hasGetKeyFrame = false;
	AVFrame *pframe = NULL;
	int dec_got_frame = -1, enc_got_frame = -1, framecnt = 0;
	while (1) {
		AVStream *in_stream, *out_stream;
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;

		pframe = av_frame_alloc();
		if (!pframe) {
			ret = AVERROR(ENOMEM);
			return -1;
		}
	/*	if (!hasGetKeyFrame&&pkt.flags == AV_PKT_FLAG_KEY)
		{
			hasGetKeyFrame = true;
			start_time = av_gettime();
		}
		else if (!hasGetKeyFrame)
		{
			av_free_packet(&pkt);
			continue;
		}*/
		ret = avcodec_decode_video2(ifmt_ctx->streams[pkt.stream_index]->codec, pframe,
			&dec_got_frame, &pkt);
		if (ret < 0) {
			av_frame_free(&pframe);
			av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
			break;
		}
		if (dec_got_frame) {
			//sws_scale(img_convert_ctx, (const uint8_t* const*)pframe->data, pframe->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

			pkt.data = NULL;
			pkt.size = 0;
			av_init_packet(&pkt);
			ret = avcodec_encode_video2(pCodecCtx, &pkt, pframe, &enc_got_frame);
			av_frame_free(&pframe);
			if (enc_got_frame == 1) {
				framecnt++;
				pkt.stream_index = video_st->index;

				//Write PTS  
				AVRational time_base = ofmt_ctx->streams[videoIndex]->time_base;//{ 1, 1000 };  
				AVRational r_framerate1 = ifmt_ctx->streams[videoIndex]->r_frame_rate;// { 50, 2 };  
				AVRational time_base_q = { 1, AV_TIME_BASE };
				//Duration between 2 frames (us)  
				int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));  //内部时间戳  
																							//Parameters  
																							//enc_pkt.pts = (double)(framecnt*calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));  
				pkt.pts = av_rescale_q(framecnt*calc_duration, time_base_q, time_base);
				pkt.dts = pkt.pts;
				pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base); //(double)(calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));  
				pkt.pos = -1;

				//Delay  
				int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
				int64_t now_time = av_gettime() - start_time;
				if (pts_time > now_time)
					av_usleep(pts_time - now_time);

				ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
				av_packet_unref(&pkt);
			}
		}
		else
		{
			av_frame_free(&pframe);
		}
	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);

end:
	if (video_st)
		avcodec_close(video_st->codec);
	avformat_free_context(ifmt_ctx);
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;
}
/*rtsp流转rtmpDemo*/
int getRtmpURLByInput(const char* in_filename, const char *out_filename)
{
	if (!in_filename || !out_filename)
	{
		fprintf(stderr, "in_filename or out_filename can not be null...");
		return -1;
	}
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVOutputFormat *ofmt = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex = -1;
	int audioindex = -1;
	int frame_index = 0;
	int audio_index = 0;
	int hasNoPts_video = 0;
	int hasNoPts_audio = 0;
	int64_t start_time = 0;

	AVDictionary* options = NULL;
	av_dict_set(&options, "rtsp_transport", "tcp", 0);
	av_dict_set(&options, "preset", "ultrafast", 0);
	av_dict_set(&options, "tune", "zerolatency", 0);

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, &options)) < 0) {
		printf("Could not open input file.");
		goto end;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++)
	{
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			//break;
		}
		else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex = i;
		}
	}
	//av_dump_format(ifmt_ctx, 0, in_filename, 0);

	//Output

	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename);

	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	//printf("%s : streams: %d\n", in_filename, ifmt_ctx->nb_streams);
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = ifmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//Copy the settings of AVCodecContext
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format------------------
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

	start_time = av_gettime();
	bool hasGetKeyFrame = false;
	int first = 0;
	while (1) {
		AVStream *in_stream, *out_stream;
		ret = av_read_frame(ifmt_ctx, &pkt); /*处理异常pkt内存问题*/
		if (ret < 0)
		{
			if (first++<3)
			{
				int64_t time = av_gettime();
				av_usleep(100);
				printf("first try value=%d\n time gap=%d\n", first,(av_gettime()-time));
				continue;
			}
			av_packet_unref(&pkt);
			//av_free_packet(&pkt);
			break;
		}
		first = 0;
		//if (!hasGetKeyFrame&&pkt.flags == AV_PKT_FLAG_KEY)
		//{
		//	hasGetKeyFrame = true;
		//	start_time = av_gettime();
		//}
		//else if (!hasGetKeyFrame)
		//{
		//	av_free_packet(&pkt);
		//	continue;
		//}
		if (pkt.pts == AV_NOPTS_VALUE) {
			if (pkt.stream_index == videoindex)
			{
				hasNoPts_video++;
				printf("no_pts_video_count: %d,current video frame index :%d\n", hasNoPts_video, frame_index);
			}
			else
			{
				hasNoPts_audio++;
				printf("no_pts_audio_count:%d,current audio frame index :%d\n", hasNoPts_audio, audio_index);
			}
			//Write PTS
			AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		}
		int pts = -1;
		int dura = -1;
		if (lastpts != -1)
		{
			pts = pkt.pts - lastpts;
			dura = pkt.duration - lastdura;
		}
		lastdura = pkt.duration;
		lastpts = pkt.pts;
		//printf("pkt msg:\n pkt streamid:%d \n pkt pts buf :%d\n pkt dura buf: %d\n pkt pts:%d\n pkt dts:%ld\n pkt duration:%ld\n", pkt.stream_index, pts, dura, pkt.pts, pkt.dts, pkt.duration);
		//Important:Delay
		if (pkt.stream_index == videoindex) {
			AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
			AVRational time_base_q = { 1,AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);
			//ifmt_ctx->streams[pkt.stream_index]->codec->width

		}


		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		/* copy packet */
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		int64_t duration = pkt.duration;
		if (pkt.duration>0x7fffffff)
		{
			//printf("%s \n", in_filename);
			pkt.duration = (int)pkt.duration;
		}
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		if (pkt.stream_index == videoindex) {
			//printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;
		}
		else
		{
			//printf("Send %8d audio frames to output URL\n", audio_index);
			audio_index++;
		}
		/*printf("%s : video_frame_count: %d\n has_nopts_video_count: %d\n audio_frame_count: %d\n hasNoPts_audio_count: %d\n",
		in_filename, frame_index, hasNoPts_video, audio_index, hasNoPts_audio);*/
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			printf("Error muxing packet\n");
			break;
		}
		//av_free_packet(&pkt);
		av_packet_unref(&pkt);
	}
	//Write file trailer
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}
	return 0;

}