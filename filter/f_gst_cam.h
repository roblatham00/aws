#ifndef _F_GST_CAM_H_
#define _F_GST_CAM_H_
// Copyright(c) 2016 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// f_gst_cam.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_gst_cam.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_gst_cam.h  If not, see <http://www.gnu.org/licenses/>. 

#include "f_base.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "../channel/ch_image.h"

class f_gst_cam: public f_base
{
 protected:
  ch_image_ref * m_ch_out;
  long long m_frm_count;
  Size m_sz;
  char m_fppl[1024];
  gchar * m_descr;
  GstElement * m_ppl;
  GstElement * m_sink;
  GstBus * m_bus;
  guint m_bus_watch_id;
  GError * m_error;
  
  void set_img(Mat & img)
  {
    m_frm_count++;
    m_ch_out->set_img(img, m_cur_time, m_frm_count);
  }

  const Size & get_sz(){
    return m_sz;
  }

 public:
  f_gst_cam(const char * name);
  virtual ~f_gst_cam();

  virtual bool init_run();
  virtual void destroy_run();
  virtual bool proc();

  static GstFlowReturn new_preroll(GstAppSink * appsink, gpointer data);
  static GstFlowReturn new_sample(GstAppSink * appsink, gpointer data);
  static gboolean bus_callback(GstBus * bus, GstMessage * message, gpointer data);
};

#endif
