#include <stddef.h>
#include <stdio.h>

#include "util.h"
#include "rtsp.h"

static int saveJPEG(AVFrame *frame, int width, int height, const char *out_file, int jpeqQuality);

static void rtspResourceInit(RTSPResource *pResource)
{
    pResource->pFormatContext = NULL;
    pResource->pCodecContext = NULL;
    pResource->pInputFrame = NULL;
}

int openIpCam(const char *ipCamResource, RTSPResource *pResource)
{
    int ret;

    rtspResourceInit(pResource);

    printfTimeDifferenceTag("0");
    // Opening the file -------------------------------------------------------
    if (avformat_open_input(&pResource->pFormatContext, ipCamResource, NULL, NULL) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't open file\n");
        return (-1);
    }

    printfTimeDifferenceTag("1");

    /// Retrieve stream information
    if (avformat_find_stream_info(pResource->pFormatContext, NULL) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Couldn't find stream information\n");
        return (-1);
    }

    // Find the first video stream --------------------------------------------
    pResource->videoStreamIndex = -1;
    for (int i = 0; i < (int)pResource->pFormatContext->nb_streams; ++i)
    {
        if (pResource->pFormatContext->streams[i]->codecpar->codec_type ==
            AVMEDIA_TYPE_VIDEO)
        {
            pResource->videoStreamIndex = i;
            pResource->frameRate = pResource->pFormatContext->streams[i]->avg_frame_rate;
            break;
        }
    }
    if (pResource->videoStreamIndex == -1)
    {
        av_log(NULL, AV_LOG_ERROR, "Didn't find a video stream\n");
        return (-1);
    }

    printfTimeDifferenceTag("2");

    return (0);
}

int openVideoCodec(RTSPResource *pResource)
{
    printfTimeDifferenceTag("4");

    AVCodec *codec = NULL;
    AVDictionary *dict = NULL;
    int ret;

    // Find the decoder for the video stream ----------------------------------
    pResource->pCodecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(pResource->pCodecContext,
                                  pResource->pFormatContext->streams[pResource->videoStreamIndex]->codecpar);
    codec = avcodec_find_decoder(pResource->pCodecContext->codec_id);
    if (codec == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Codec not found\n");
        return (-1);
    }
    pResource->pCodecContext->codec = codec;

    // Open video decoder ------------------------------------------------------
    ret = avcodec_open2(pResource->pCodecContext, codec, &dict);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open codec error\n");
        return (-1);
    }

    pResource->pInputFrame = av_frame_alloc();

    printfTimeDifferenceTag("5");

    return (0);
}

static int decodePacketToFrame(AVCodecContext *avctx, AVFrame *frame, int *pGotFrame, AVPacket *pkt)
{
    int ret;

    *pGotFrame = 0;

    if (pkt)
    {
        printfTimeDifferenceTag("8");
        ret = avcodec_send_packet(avctx, pkt);
        if (ret < 0 && ret != AVERROR_EOF)
            return ret;
    }

    printfTimeDifferenceTag("9");

    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN))
        return ret;
    if (ret >= 0)
        *pGotFrame = 1;

    printfTimeDifferenceTag("10");
    return 0;
}

// takes a picture after a delay
int takeAPicture(RTSPResource *pResource,
                 const char *pictureName,
                 int jpeqQuality,
                 int delay)
{
    // delay in frames
    int frameDelay = delay * pResource->frameRate.num;

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    printfTimeDifferenceTag("6");
    // read a packet from the stream -------------------------------------------
    while ((av_read_frame(pResource->pFormatContext, &packet) >= 0))
    {
        printfTimeDifferenceTag("7");

        if (packet.stream_index == pResource->videoStreamIndex)
        {
            int gotFrame, ret;
            // decode packet to frame ------------------------------------------
            ret = decodePacketToFrame(pResource->pCodecContext, pResource->pInputFrame, &gotFrame, &packet);

            if (gotFrame == 1)
            {

                if ((frameDelay == 0) && (pictureName != NULL))
                {
                    printfTimeDifferenceTag("11");
                    // save video frame as a jpeg image --------------------
                    saveJPEG(pResource->pInputFrame,
                             pResource->pCodecContext->width,
                             pResource->pCodecContext->height,
                             pictureName, jpeqQuality);

                    printfTimeDifferenceTag("12");
                }

                av_packet_unref(&packet);

                // flushing the codec ------------------------------------------
                decodePacketToFrame(pResource->pCodecContext, pResource->pInputFrame, &gotFrame, NULL);

                if (frameDelay == 0)
                {
                    break;
                }
                frameDelay--;
            }
            if (ret < 0)
            {
                avcodec_flush_buffers(pResource->pCodecContext);
            }
        }
        av_packet_unref(&packet);
    }

    return (0);
}

void closeVideoCodec(RTSPResource *pResource)
{
    av_free(pResource->pInputFrame);
    avcodec_free_context(&pResource->pCodecContext);
}

void closeIpCam(RTSPResource *pResource)
{

    avformat_close_input(&pResource->pFormatContext);
}

static int saveJPEG(AVFrame *frame, int width, int height, const char *out_file, int jpeqQuality)
{
    // Create a new one Output AVFormatContext and allocate memory
    AVFormatContext *output_cxt = NULL;
    avformat_alloc_output_context2(&output_cxt, NULL, "singlejpeg", out_file);

    // Create and initialize an AVIOContext related to the URL
    if (avio_open(&output_cxt->pb, out_file, AVIO_FLAG_READ_WRITE) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file\n");
        return -1;
    }

    // Build a new Stream
    AVStream *stream = avformat_new_stream(output_cxt, NULL);
    if (stream == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to create AVStream\n");
        return -1;
    }
    // Initialize AVStream information
    AVCodecContext *codec_cxt = stream->codec;
    codec_cxt->codec_id = output_cxt->oformat->video_codec;
    codec_cxt->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_cxt->pix_fmt = AV_PIX_FMT_YUVJ420P;
    codec_cxt->height = height;
    codec_cxt->width = width;
    codec_cxt->time_base.num = 1;
    codec_cxt->time_base.den = 15;
    codec_cxt->flags |= AV_CODEC_FLAG_QSCALE;
    codec_cxt->global_quality = FF_QP2LAMBDA * jpeqQuality;
    frame->quality = codec_cxt->global_quality;
    //    frame->pict_type = 0;

    avcodec_parameters_from_context(stream->codecpar, codec_cxt);

    AVCodec *codec = avcodec_find_encoder(codec_cxt->codec_id);
    if (!codec)
    {
        av_log(NULL, AV_LOG_ERROR, "Encoder not found\n");
        return -1;
    }

    if (avcodec_open2(codec_cxt, codec, NULL) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Cannot open the encoder\n");
        return -1;
    }

    // Print output file information
    av_dump_format(output_cxt, 0, out_file, 1);

    // Write the file header, frame and trailer ------------------------------
    avformat_write_header(output_cxt, NULL);
    int size = codec_cxt->width * codec_cxt->height;

    AVPacket packet;
    av_new_packet(&packet, size * 3);

    avcodec_send_frame(codec_cxt, frame);
    if (avcodec_receive_packet(codec_cxt, &packet) == 0)
    {
        av_write_frame(output_cxt, &packet);
    }
    av_write_trailer(output_cxt);

    // release resources ------------------------------------------------------
    av_packet_unref(&packet);
    avio_close(output_cxt->pb);
    avformat_free_context(output_cxt);
    return 0;
}
