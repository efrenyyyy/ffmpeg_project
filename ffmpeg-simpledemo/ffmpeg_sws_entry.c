#define USEFILTER 1

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#define snprintf _snprintf
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#if USEFILTER
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#endif
};

void ffmpeg_vf_decode_and_encode()
{
    AVFormatContext *ifmt_ctx, *ofmt_ctx;
    AVCodecContext *codec_ctx;
    AVCodec *codec;
    AVPacket *dec_pkt;
    AVFrame *enc_frame;
    AVFrame *pFrame, *pFrameYUV;
    AVInputFormat *fmt;
    AVBitStreamFilterContext *bsfc;
    int64_t start_time = av_gettime();
    char *src_name, *dst_name, *out_buffer;
    uint8_t *buffer, *bufferYUV;
    int got_picture;
    int video_index = -1;

    src_name = "rtmp://10.10.13.98/live/lb_lvyounew_high";
    dst_name = "rtmp://10.121.86.127/live/transform";

    av_register_all();
    avcodec_register_all();

    avformat_network_init();
    ifmt_ctx = avformat_alloc_context();
    if (!ifmt_ctx)
    {
        return;
    }
    //ofmt_ctx = avformat_alloc_context();
    //if (!ofmt_ctx)
    //{
    //    return;
    //}
    fmt = av_find_input_format("flv");
    if (avformat_open_input(&ifmt_ctx, src_name, fmt, NULL) < 0)
    {
        return;
    }
    if (avformat_find_stream_info(ifmt_ctx, NULL) < 0)
    {
        return;
    }
    for (size_t i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }
    if (video_index == -1)
    {
        return;
    }
    codec_ctx = ifmt_ctx->streams[video_index]->codec;
    enc_frame = av_frame_alloc();
    if (!enc_frame)
    {
        return;
    }
    dec_pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_init_packet(dec_pkt);
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (!codec)
    {
        return;
    }
    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
        return;
    }
    
    pFrameYUV = av_frame_alloc();
    int bytes_num = avpicture_get_size(AV_PIX_FMT_YUV420P, 960, 540);
    bufferYUV = (uint8_t *)av_malloc(bytes_num);
    avpicture_fill((AVPicture *)pFrameYUV, bufferYUV, AV_PIX_FMT_YUV420P, 960, 540);

    //pFrame = av_frame_alloc();
    //bytes_num = avpicture_get_size(AV_PIX_FMT_YUV422P, 720, 540);
    //buffer = (uint8_t *)av_malloc(bytes_num);
    //avpicture_fill((AVPicture *)pFrame, buffer, AV_PIX_FMT_YUV422P, 720, 540);


    pFrame = av_frame_alloc();
    bytes_num = avpicture_get_size(AV_PIX_FMT_YUV420P, 960, 540);
    buffer = (uint8_t *)av_malloc(bytes_num);
    avpicture_fill((AVPicture *)pFrame, buffer, AV_PIX_FMT_YUV420P, 960, 540);

    FILE *file = fopen("test-yuv420p.264", "wb");
    struct SwsContext *sws_ctx;
    bsfc = av_bitstream_filter_init("h264_mp4toannexb");
    int count = 0;
    while (av_read_frame(ifmt_ctx, dec_pkt) >= 0)
    {
        if (dec_pkt->stream_index != video_index)
        {
            av_free_packet(dec_pkt);
            continue;
        }

        av_bitstream_filter_filter(bsfc, codec_ctx, NULL, &dec_pkt->data, &dec_pkt->size, dec_pkt->data, dec_pkt->size, dec_pkt->flags&AV_PKT_FLAG_KEY);
        if (avcodec_decode_video2(codec_ctx, pFrame, &got_picture, dec_pkt) < 0)
        {
            //av_frame_free(&pFrame);
            av_free_packet(dec_pkt);
            continue;
        }

        if (got_picture)
        {
            fwrite(dec_pkt->data, 1, dec_pkt->size, file);
            count++;
            printf("total frames:%d\n", count);
        }
        else
        {
            //av_free_packet(dec_pkt);
            //break;
        }
        av_free_packet(dec_pkt);
        //sws_ctx = sws_getContext(960, 540, AV_PIX_FMT_YUV420P, 720, 540, AV_PIX_FMT_YUV422P, SWS_BICUBIC, NULL, NULL, NULL);
        //if (!sws_ctx)
        //    break;
        //sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 960, 540, pFrameYUV->data, pFrameYUV->linesize);
        //sws_freeContext(sws_ctx);
        //fwrite(pFrameYUV->data[0], 1, pFrameYUV->linesize[0], file);
        //fwrite(pFrameYUV->data[1], 1, pFrameYUV->linesize[1], file);
        //fwrite(pFrameYUV->data[2], 1, pFrameYUV->linesize[2], file);
    }
    av_bitstream_filter_close(bsfc);
    fclose(file);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    av_frame_free(&enc_frame);
    avformat_free_context(ifmt_ctx);
    //avformat_free_context(ofmt_ctx);

}

int main()
{
    ffmpeg_vf_decode_and_encode();
}
