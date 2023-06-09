/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>

#include <gst/rtsp-server/rtsp-server.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/aruco.hpp>

using namespace cv;

typedef struct
{
  gboolean white;
  GstClockTime timestamp;
  Ptr<aruco::Dictionary> dictionary;
  Ptr<aruco::GridBoard> board;
} MyContext;

Size imageSize{384,288};

int dictionaryId = 10;
int margin = 10;
int borderBits = 1;


/* called when we need to give data to appsrc */
static void
need_data (GstElement * appsrc, guint unused, MyContext * ctx)
{
  GstBuffer *buffer;
  guint size;
  GstFlowReturn ret;

  // Mat boardImage(385, 288, CV_8UC1, Scalar(0, 0, 0));
  Mat boardImage = Mat(384, 288, CV_8UC1, Scalar::all(255));
  cv::Mat ar_img;
  cv::aruco::drawMarker(ctx->dictionary, ctx->white ? 1 : 0, 100, ar_img);
  // cv::Mat mat = (cv::Mat_<double>(2,3)<<1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  double angle = -50.0;
  double scale = 1.0;
  cv::Mat mat = getRotationMatrix2D( center, angle, scale );
  cv::warpAffine(ar_img, boardImage, mat, boardImage.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
  // cv::Rect roi( cv::Point( 0, 0 ), ar_img.size() );
  // ar_img.copyTo(boardImage(roi));
  cv::Mat rgb_img;
  cv::Mat yuv_img;
  cv::cvtColor(boardImage, rgb_img, cv::COLOR_GRAY2RGB);
  cv::cvtColor(rgb_img, yuv_img, cv::COLOR_RGB2YUV_I420);

  // ctx->board->draw(imageSize, boardImage, margin, borderBits);
  size = yuv_img.total() * yuv_img.elemSize();
  buffer = gst_buffer_new_allocate (NULL, size, NULL);

  // boardImage = Scalar(ctx->white ? 0xFF : 0x00, ctx->white ? 0x0A : 0xCF, ctx->white ? 0x6C : 0x13);

  guchar* data1 = yuv_img.data;
  GstMapInfo map;
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  // memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );
  memcpy( (guchar *)map.data, data1, size);

  /* this makes the image black/white */
  // gst_buffer_memset (buffer, 0, ctx->white ? 0xff : 0x0, size);
  
  /*
  GstMemory* wrapped_mem1 = gst_memory_new_wrapped(
        (GstMemoryFlags)0,
        (gpointer)boardImage.data,
        size,
        0,
        size,
        NULL,
        NULL // (GDestroyNotify)g_free
  );
  gst_buffer_append_memory(buffer, wrapped_mem1);
  */

  ctx->white = !ctx->white;

  /* increment the timestamp every 1/2 second */
  GST_BUFFER_PTS (buffer) = ctx->timestamp;
  GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
  ctx->timestamp += GST_BUFFER_DURATION (buffer);

  g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
  gst_buffer_unref (buffer);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
    gpointer user_data)
{
  GstElement *element, *appsrc;
  MyContext *ctx;

  /* get the element used for providing the streams of the media */
  element = gst_rtsp_media_get_element (media);

  /* get our appsrc, we named it 'mysrc' with the name property */
  appsrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "mysrc");

  /* this instructs appsrc that we will be dealing with timed buffer */
  gst_util_set_object_arg (G_OBJECT (appsrc), "format", "time");
  /* configure the caps of the video */
  g_object_set (G_OBJECT (appsrc), "caps",
      gst_caps_new_simple ("video/x-raw",
          "format", G_TYPE_STRING, "I420",
          "width", G_TYPE_INT, 288,
          "height", G_TYPE_INT, 384,
          "framerate", GST_TYPE_FRACTION, 0, 1, NULL), NULL);

  ctx = g_new0 (MyContext, 1);
  ctx->dictionary = aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME(dictionaryId));
  ctx->board = aruco::GridBoard::create(2, 1, 0.04f, 0.01f, ctx->dictionary);

  ctx->white = FALSE;
  ctx->timestamp = 0;
  /* make sure ther datais freed when the media is gone */
  g_object_set_data_full (G_OBJECT (media), "my-extra-data", ctx,
      (GDestroyNotify) g_free);

  /* install the callback that will be called when a buffer is needed */
  g_signal_connect (appsrc, "need-data", (GCallback) need_data, ctx);
  gst_object_unref (appsrc);
  gst_object_unref (element);
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points (server);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines.
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory,
      "( appsrc name=mysrc ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )");

  /* notify when our media is ready, This is called whenever someone asks for
   * the media and a new pipeline with our appsrc is created */
  g_signal_connect (factory, "media-configure", (GCallback) media_configure,
      NULL);

  /* attach the test factory to the /test url */
  gst_rtsp_mount_points_add_factory (mounts, "/test", factory);

  /* don't need the ref to the mounts anymore */
  g_object_unref (mounts);

  /* attach the server to the default maincontext */
  gst_rtsp_server_attach (server, NULL);

  /* start serving */
  g_print ("stream ready at rtsp://127.0.0.1:8554/test\n");
  g_main_loop_run (loop);

  return 0;
}
