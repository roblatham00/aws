// Copyright(c) 2016 Yohei Matsumoto, All right reserved. 

// c_aws1_ui_map.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// c_aws1_ui_map.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with c_aws1_ui_map.cpp.  If not, see <http://www.gnu.org/licenses/>. 

#include "stdafx.h"
#include <cstdio>
#include <cstring>
#include <cmath>

#include <iostream>
#include <fstream>
#include <list>
#include <map>
using namespace std;

#include "../util/aws_stdlib.h"
#include "../util/aws_thread.h"
#include "../util/c_clock.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <opencv2/opencv.hpp>
using namespace cv;

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <GL/glut.h>
#include <GL/glu.h>

#include "../util/aws_glib.h"
#include "f_aws1_ui.h"

/////////////////////////////////////////////////////////////////////////// c_aws1_ui_map

c_aws1_ui_map::c_aws1_ui_map(f_aws1_ui * _pui):c_aws1_ui_core(_pui), 
	m_aws1_waypoint_file_version("#aws1_waypoint_v_0.00"), m_op(EMO_EDT_RT),
	m_rt_sv(0), m_rt_ld(0), m_map_range(1000)
{
	m_cur_pos.x = m_cur_pos.y = 0.;
	m_map_pos.x = m_map_pos.y = 0.;

	// points for the ship object
	m_ship_pts[0].x = 0.f;
	m_ship_pts[0].y = 1.f;
	m_ship_pts[1].x = 0.5f;
	m_ship_pts[1].y = -1.f;
	m_ship_pts[2].x = -0.5f;
	m_ship_pts[2].y = -1.f;

	// calculating circular points
	const float step = (float)(2 * PI / 36.);
	float theta = 0.;
	for (int i = 0; i < 36; i++, theta += step){
		m_circ_pts[i].x = (float)cos(theta);
		m_circ_pts[i].y = (float)sin(theta);
	}
}


void c_aws1_ui_map::js(const s_jc_u3613m & js)
{
	const Size & sz_win = get_window_size();

	if(sz_win.width > sz_win.height){ // 1 in y direction equals to m_map_range
		fx = (float)((float) sz_win.height / (float) sz_win.width);
		fy = 1.0;
	}else{ // 1 in x direction equals to m_map_range
		fx = 1.0;
		fy = (float)((float) sz_win.width / (float) sz_win.height);
	}

	m_imap_range = (float)(1.0 / m_map_range);
	fxmeter = fx * m_map_range;
	fymeter = fy * m_map_range;
	ifxmeter = (float)(1.0 / fxmeter);
	ifymeter = (float)(1.0 / fymeter);

	float lat, lon, alt, galt;
	long long t = 0;
	ch_state * pstate = get_ch_state();
	pstate->get_position(t, lat, lon, alt, galt);
	pstate->get_position_ecef(t, Porg.x, Porg.y, Porg.z);
	Rorg = pstate->get_enu_rotation(t);

	// Operation selection (cross key)
	
	// Map move (left stick)	
	m_map_pos.x += (float)(js.lr1 * fxmeter * (1.0/60.0));
    m_map_pos.y -= (float)(js.ud1 * fymeter * (1.0/60.0) );	
	offset = Point2f((float)(-m_map_pos.x * ifxmeter), (float)(-m_map_pos.y * ifymeter));

	if(js.elst & s_jc_u3613m::EB_STDOWN){ // back to the own ship 
		m_map_pos.x = m_map_pos.y = 0.;
	}

	// calculating the map center positioin in three coordinate
	wrldtoecef(Rorg, Porg.x, Porg.y, Porg.z, m_map_pos.x, m_map_pos.y, 0.f, mp_x, mp_y, mp_z);
	eceftobih(mp_x, mp_y, mp_z, mp_lat, mp_lon, mp_alt);
	{
		ch_map* pmap = get_ch_map();
		if (pmap){
			pmap->set_center(mp_x, mp_y, mp_z);
		}
	}

	// Cursor move (right stick)
	m_cur_pos.x += (float)(js.lr2 * fx * (1.0/60.0));
    m_cur_pos.y -= (float)(js.ud2 * fy * (1.0/60.0));		
	m_cur_pos.x = max(-1.f, m_cur_pos.x);
	m_cur_pos.x = min(1.f, m_cur_pos.x);
	m_cur_pos.y = max(-1.f, m_cur_pos.y);
	m_cur_pos.y = min(1.f, m_cur_pos.y);

	// calculating the cursor position in three coordinate
	cp_rx = (float)(m_cur_pos.x * fxmeter + m_map_pos.x);
	cp_ry = (float)(m_cur_pos.y * fymeter + m_map_pos.y);

	wrldtoecef(Rorg, Porg.x, Porg.y, Porg.z, cp_rx, cp_ry, 0.f, cp_x, cp_y, cp_z);
	eceftobih(cp_x, cp_y, cp_z, cp_lat, cp_lon, cp_alt);

	switch(m_op){
	case EMO_EDT_RT:
		if(js.is_event_down(js.elx)){
			ch_wp * pwp = get_ch_wp();
			pwp->lock();
			pwp->prev_focus();
			pwp->unlock();
		}else if(js.is_event_down(js.erx)){
			ch_wp * pwp = get_ch_wp();
			pwp->lock();
			pwp->next_focus();
			pwp->unlock();
		}

		if(js.is_event_down(js.eb)){
			ch_wp * pwp = get_ch_wp();
			pwp->lock();
			pwp->ers();
			pwp->unlock();
		}else if(js.is_event_down(js.ea)){
			ch_aws1_ap_inst * ap_inst = get_ch_ap_inst();
			if (ap_inst){
				switch (ap_inst->get_mode()){
				case EAP_CURSOR:
				case EAP_STAY:
				{
								 ap_inst->set_stay_pos(cp_lat, cp_lon);
								 break;
				}
				case EAP_WP:
				{
							   ch_wp * pwp = get_ch_wp();
							   /*
							   s_wp wp;
							   wp.rx = cp_rx;
							   wp.ry = cp_ry;
							   wp.rz = 0.;
							   wp.x = cp_x;
							   wp.y = cp_y;
							   wp.z = cp_z;
							   wp.lat = cp_lat;
							   wp.lon = cp_lon;
							   wp.rarv = 10.; // 10meter
							   */
							   pwp->lock();
							   pwp->ins(cp_lat, cp_lon, 10.);
							   pwp->unlock();
				}
				}
			}else{
				ch_wp * pwp = get_ch_wp();
				/*
				s_wp wp;
				wp.rx = cp_rx;
				wp.ry = cp_ry;
				wp.rz = 0.;
				wp.x = cp_x;
				wp.y = cp_y;
				wp.z = cp_z;
				wp.lat = cp_lat;
				wp.lon = cp_lon;
				wp.rarv = 10.; // 10meter
				*/
				pwp->lock();
				pwp->ins(cp_lat, cp_lon, 10.);
				pwp->unlock();
			}
		}
		break;
	case EMO_SV_RT:	
		if(js.is_event_down(js.elx)){
			m_rt_sv--;
			if(m_rt_sv < 0)
				m_rt_sv += MAX_RT_FILES;
		}else if(js.is_event_down(js.erx)){
			m_rt_sv++;
			if(m_rt_sv >= MAX_RT_FILES)
				m_rt_sv -= MAX_RT_FILES;
		}
		if(js.is_event_down(js.ea)){
			char fname[1024];
			snprintf(fname, 1024, "%s/%03d.rt", get_path_storage(), m_rt_sv);
			FILE * pf = fopen(fname, "w");
			
			if(pf){
				fprintf(pf, "%s\n", m_aws1_waypoint_file_version);
			
				ch_wp * pwp = get_ch_wp();
				fprintf(pf, "%d\n", pwp->get_num_wps());
				pwp->lock();
				int i = 0;
				for(pwp->begin();!pwp->is_end(); pwp->next()){
					s_wp & wp = pwp->cur();
					fprintf(pf, "%d %013.8f %013.8f %03.1f\n", i, (float)(wp.lat * (180 / PI)),
						(float)(wp.lon * (180 / PI)), wp.rarv);
					i++;
				}
				pwp->unlock();
				fclose(pf);
			}else{
				cerr << "Failed to save route file " << fname << "." << endl;
			}			
		}
		break;
	case EMO_LD_RT:
		if(js.is_event_down(js.elx)){
			m_rt_ld--;
			if(m_rt_ld < 0)
				m_rt_ld += MAX_RT_FILES;
		}else if(js.is_event_down(js.erx)){
			m_rt_ld++;
			if(m_rt_ld >= MAX_RT_FILES)
			  m_rt_ld -= MAX_RT_FILES;
		}

		if(js.is_event_down(js.ea)){
			char fname[1024];
			snprintf(fname, 1024, "%s/%03d.rt", get_path_storage(), m_rt_ld);
			FILE * pf = fopen(fname, "r");

			if(pf){
				bool valid = true;
				const char * p = m_aws1_waypoint_file_version;
				char c;
				for(c = fgetc(pf); c != EOF && c != '\n'; c = fgetc(pf), p++){
					if(*p != c){
						valid = false;
					}
				}
				if(c == '\n' && valid){
					ch_wp * pwp = get_ch_wp();
					int num_wps;
					if(EOF == fscanf(pf, "%d\n", &num_wps)){
						cerr << "Unexpected end of file was detected in " << fname << "." << endl;
						break;
					}
					float lat, lon, rarv;
					pwp->lock();
					pwp->clear();

					for(int i = 0; i < num_wps; i++){
						int _i;
						if(EOF == fscanf(pf, "%d %f %f %f\n", &_i, &lat, &lon, &rarv)){
							cerr << "Unexpected end of file was detected in " << fname << "." << endl;
							break;
						}
						pwp->ins((float)(lat * (PI / 180.)), (float)(lon * (PI / 180.)), rarv);
					}
					pwp->unlock();
				}else{
					cerr << "The file " << fname << " does not match the current waypoint file version." << endl;
				}
				fclose(pf);
			}else{
				cerr << "Failed to save route file " << fname << "." << endl;
			}			
		}
		break;
	case EMO_RNG:
		if(js.is_event_down(js.elx)){
			m_map_range *= 0.1f;
			m_imap_range *= 10.f;
		}else if(js.is_event_down(js.erx)){
			m_map_range *= 10.f;
			m_imap_range *= 0.1f;
		}
		break;
	}

	if(js.is_event_down(js.eux)){
		m_op = (e_map_operation) (m_op == EMO_EDT_RT ? EMO_RNG : m_op - 1);
	}else if(js.is_event_down(js.edx)){
		m_op = (e_map_operation) ((m_op + 1) % (EMO_RNG + 1));
	}

	if(js.is_event_down(js.elb)){
		m_map_range *= 0.1f;
		m_imap_range *= 10.f;
	}else if(js.is_event_down(js.erb)){
		m_map_range *= 10.f;
		m_imap_range *= 0.1f;
	}

	if(m_map_range > 100000000){
		m_map_range = 100000000.f;
		m_imap_range = (float)(1. / 100000000.);
	}else if(m_map_range < 100){
		m_map_range = 100.f;
		m_imap_range = (float)(1. / 100.);
	}
}

void c_aws1_ui_map::draw_coast_line(const vector<Point3f> & cl, float lw)
{
	glColor4f(0, 1, 0, 1);
	glLineWidth(lw);
	glBegin(GL_LINE_STRIP);
	double * pR = Rorg.ptr<double>();

	for(int i = 0; i < cl.size(); i++){
		Point3f pt3d = cl[i] - Porg;
		Point2f pt2d;
		pt2d.x = (float)((pt3d.x * pR[0] + pt3d.y * pR[1] + pt3d.z * pR[2]) * ifxmeter + offset.x);
		pt2d.y = (float)((pt3d.x * pR[3] + pt3d.y * pR[4] + pt3d.z * pR[5]) * ifymeter + offset.y);
		glVertex2f(pt2d.x, pt2d.y);
	}
	glEnd();
}

void c_aws1_ui_map::draw_waypoints(const float wfont, const float lw)
{
	bool prev = false;
	Point2f pos_prev;
	ch_aws1_ap_inst * ap_inst = get_ch_ap_inst();
	ch_wp * pwp = get_ch_wp();

	Point2f pts[36]; // scaled points
	for (int i = 0; i < 36; i++){
		pix2nml((float)(m_circ_pts[i].x * 10.), (float)(m_circ_pts[i].y * 10.),
			pts[i].x, pts[i].y);
	}

	e_ap_mode ap_mode = EAP_WP;
	if (ap_inst){
		ap_mode = ap_inst->get_mode();
	}
	float clr_wp = 1.0;
	switch (ap_mode){
	case EAP_CURSOR:
	case EAP_STAY:
	{
					 float srx, sry, d, dir;
					 if (ap_inst->get_stay_pos_rel(srx, sry, d, dir)){
						 Point2f pos((float)((srx * ifxmeter) + offset.x),
							 (float)((sry * ifymeter) + offset.y));
						 drawGlPolygon2Df(pts, 36, pos, 0, 1.0, 0, 0, lw);
						 drawGlLine2Df(pos.x, pos.y, offset.x, offset.y, 0, 1.0, 0., 1., lw);
						 char str[32];
						 snprintf(str, 32, "D%04.1f,C%04.1f", d, dir);
						 drawGlText(pos.x + wfont, pos.y, str, 0, 1.0, 0, 1.0, GLUT_BITMAP_8_BY_13);
					 }
	}
		clr_wp = 0.5;
		break;
	case EAP_WP:
		clr_wp = 1.0;
		break;
	}

	if (pwp){
		pwp->lock();

		int num_wps = pwp->get_num_wps();

		// drawing waypoint
		pwp->begin();
		for (; !pwp->is_end(); pwp->next()){
			s_wp & wp = pwp->cur();
			if (!wp.update_rpos)
				continue;
			Point2f pos;
			pos.x = (float)((wp.rx * ifxmeter) + offset.x);
			pos.y = (float)((wp.ry * ifymeter) + offset.y);

			if (pwp->is_focused()){
				drawGlPolygon2Df(pts, 36, pos, 0, clr_wp, 0, 0, lw);
			}
			else{
				drawGlPolygon2Df(pts, 36, pos, 0, 0.5, 0, 0, lw);
			}

			if (prev)
				drawGlLine2Df(pos_prev.x, pos_prev.y, pos.x, pos.y, 0, 0.5, 0, 0, lw);

			long long tarr = wp.get_arrival_time();
			if (tarr > 0){
				tarr -= get_cur_time();
				char str[32];
				snprintf(str, 32, "%lld", tarr / SEC);
				drawGlText(pos.x + wfont, pos.y, str, 0, 1.0, 0, 1.0, GLUT_BITMAP_8_BY_13);
			}

			prev = true;
			pos_prev = pos;
		}

		// checking waypoint arrival
		if (!pwp->is_finished()){
			s_wp & wp = pwp->get_next_wp();
			Point2f pos;
			pos.x = (float)((wp.rx * ifxmeter) + offset.x);
			pos.y = (float)((wp.ry * ifymeter) + offset.y);
		
			float d, cdiff;
			char str[32];
			pwp->get_diff(d, cdiff);
			snprintf(str, 32, "D%04.1f,C%04.1f", d, cdiff);
			drawGlText(pos.x + wfont, pos.y, str, 0, 1.0, 0, 1.0, GLUT_BITMAP_8_BY_13);
			draw_circle(pos, wp.rarv, 1.0, 1.0, 0, 1.0, lw);

			if (ap_mode == EAP_WP)
				drawGlLine2Df(pos.x, pos.y, offset.x, offset.y, 0, 1.0, 0., 1., lw);
		}

		pwp->unlock();
	}

}

void c_aws1_ui_map::draw()
{
  float cog, sog, roll, pitch, yaw;
  float nvx, nvy, vx, vy;
  Point3f cecef;
  long long t = 0;
  ch_state * pstate = get_ch_state();
  pstate->get_velocity(t, cog, sog);
  pstate->get_attitude(t, roll, pitch, yaw);
  pstate->get_position_ecef(t, cecef.x, cecef.y, cecef.z);
  pstate->get_norm_velocity_vector(t, nvx, nvy);
  pstate->get_velocity_vector(t, vx, vy);

  float wfont = 8, hfont = 13;
  pix2nml(wfont, hfont, wfont, hfont);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	float lw;
	pix2nml(1, 1, lw, lw);

	// draw map
	{
		ch_map * pmap = get_ch_map();
		if(pmap){
			pmap->lock();
			int ilv = (int)(log10(m_map_range * 0.001) + 0.5);

			const list<const vector<Point3f>*> & lines = pmap->get_lines(ilv);
			for (list<const vector<Point3f>*>::const_iterator itr = lines.begin();
				itr != lines.end(); itr++){
				const vector<Point3f> & line = *(*itr);
				draw_coast_line(line, lw);

			}
			pmap->unlock();
		}
	}

	// draw range circle 
	{
		Point2f pts[36]; // scaled points
		for(int i = 0; i < 36; i++){
			pts[i].x = m_circ_pts[i].x * fx;
			pts[i].y = m_circ_pts[i].y * fy;
		}

		drawGlPolygon2Df(pts, 36, 0, 1, 0, 0, lw); // own ship triangle
	}
	
	// draw own ship
	{
		Point2f pts[3]; // rotated version of the ship points

		float theta = (float)(yaw * (PI / 180.));
		float c = (float) cos(theta), s = (float) sin(theta);
		float ws, hs;
		pix2nml(10., 10., ws, hs);

		for(int i = 0; i < 3; i++){
			// rotating the points
			// multiply [ c -s; s c]
			pts[i].x = (float)(ws * (m_ship_pts[i].x * c + m_ship_pts[i].y * s) + offset.x);
			pts[i].y = (float)(hs * (- m_ship_pts[i].x * s + m_ship_pts[i].y * c) + offset.y);
		}

		drawGlPolygon2Df(pts, 3, 0, 1, 0, 0, lw); // own ship triangle

		vy = vy * 180. * ifymeter; 
		vx = vx * 180. * ifxmeter;
		drawGlLine2Df(offset.x, offset.y, (float)(offset.x + vx), (float)(offset.y + vy), 0., 1., 0., 0., lw);
	}

	// draw waypoints
	draw_waypoints(wfont, lw);

	// draw objects
	{
		/*
		ch_obj * pobj = get_obj();
		if(pobj){
			for(pobj->begin();!pobj->is_end(); pobj->next()){
				c_obj & obj = *pobj->cur();
				if(obj.get_type() & EOT_SHIP){				
					draw_ship_object(obj);
				}
			}
		}
		*/
	}

// draw ais objects
	{
		ch_ais_obj * pobj = get_ch_ais_obj();
		if (pobj){
			pobj->lock();
			float range = (float)(0.5 * m_map_range);

			for (pobj->begin(); !pobj->is_end(); pobj->next()){
				{ // range test
					float bear = 0.f, dist = 0.f;
					if (!pobj->get_pos_bd(bear, dist) || dist > range)
						continue;
				}

				float x = 0.f, y = 0.f, z = 0.f, vx = 0.f, vy = 0.f, vz = 0.f,
					yw = 0.f, tcpa = 0, dcpa = 0, xp = 0, yp = 0, sp = 0;
				if (pobj->get_cur_state(x, y, z, vx, vy, vz, yw)){
					if (pobj->get_tdcpa(tcpa, dcpa)
						&& pobj->get_prediction(t, xp, yp, sp))
						draw_ship_object(x, y, z, vx, vy, vz, yw,tcpa, dcpa, xp, yp, sp);
					else
						draw_ship_object(x, y, z, vx, vy, vz, yw);
				}
			}
			pobj->unlock();
		}
	}

// draw obstacles
	{
		ch_obst * pch_obst = get_ch_obst();
		if (pch_obst){
			Point2f pts[36]; // scaled points
			pch_obst->lock();
			for (pch_obst->begin(); !pch_obst->is_end(); pch_obst->next()){
				c_obst * pobst = pch_obst->cur();
				float x, y, z, r;
				if (!pobst->get_pos_rel(x, y, z))
					continue;

				r = pobst->get_rad();
				Point2f pos;
				pos.x = (float)((x * ifxmeter) + offset.x);
				pos.y = (float)((y * ifymeter) + offset.y);
				
				draw_circle(pos, r, (float)min(1.0, pobst->get_update_count() * 0.1), 0, 0, 0, lw);
			}
			pch_obst->unlock();
		}
	}

	// draw cursor
	{
		float x1, x2, y1, y2;
		x1 = y1 = 16.;
		pix2nml(x1, y1, x1, y1);
		x2 = x1;
		y2 = y1;
		x1 = m_cur_pos.x - x1;
		x2 = m_cur_pos.x + x2;
		y1 = m_cur_pos.y - y1;
		y2 = m_cur_pos.y + y2;
		drawGlSquare2Df(x1, y1, x2, y2, 0, 1., 0., 0.5, lw);
		drawGlLine2Df(x1, m_cur_pos.y, x2, m_cur_pos.y, 0., 1., 0., 1., 1.);
		drawGlLine2Df(m_cur_pos.x, y1, m_cur_pos.x, y2, 0., 1., 0., 1., 1.);
	}

	// draw operation lists (right bottom)
	draw_ui_map_operation(wfont, hfont, lw);

	// draw Engine and Rudder status 
	pui->ui_show_meng();
	pui->ui_show_seng();
	pui->ui_show_rudder();

	// draw own ship status
	pui->ui_show_state();
}

void c_aws1_ui_map::draw_ui_map_operation(float wfont, float hfont, float lw)
{
   	// (x0, y0)                        <-24chars
	// |------------------------------| 
	// |            Menu              |
	// |------------------------------|y0-2hfont
	// |  Edit Route  | <selected WP> |
	// |  Save Route  | <File Index>  |
	// |  Load Route  | <File Index>  |
	// |  Range       | < Range >     |
	// |------------------------------|y0-8.5hfont
	// | Button Explantion            |
	// |------------------------------| <- Right end
	//                ^|              ^|
	//                12chars           Bottom end (1, -1) = (x1, y1)

	const char * title = "Menu";
	const char * items[4] = {
		"Edit Route",
		"Save Route", 
		"Load Route", 
		"Range"
	}; // max 10 chars
	const char * mexp[4] = {
		"Select(<- ->), Add(a), Del(b)",
		"Select(<- ->), Save(a)",
		"Select(<- ->), Load(a)",
		"0.1x (<-, LB), 10x(->, RB)"
	}; // max 29 chars

	// calculate the box's position and scale
	float width = (float)(wfont * 31);
	float height = (float)(hfont * 10.5);

	float x0 = (float)(1. - width), y0 = (float)(-1.0 + height); 
	float x1 = 1.0, y1 = -1.0;
	float x, y, xv;

	drawGlSquare2Df(x0, y0, x1, y1, 0, 0, 0, 1);
	drawGlSquare2Df(x0, y0, x1, y1, 0, 1, 0, 1, lw);
	drawGlLine2Df(x0, y0, x1, y0, 0, 1, 0, 1, lw);
	y = (float)(y0 - 2 * hfont);
	drawGlLine2Df(x0, y, x1, y, 0, 1, 0, 1, lw);
	y = (float)(y0 - 8.5 * hfont);
	drawGlLine2Df(x0, y, x1, y, 0, 1, 0, 1, lw);
	y = y1;
	drawGlLine2Df(x0, y, x1, y, 0, 1, 0, 1, lw);

	x = (float)(10 * wfont + x0);
	y = (float)(y0 - 1.5 * hfont);
	drawGlText(x, y, title, 0, 1, 0, 1, GLUT_BITMAP_8_BY_13);

	x = (float)(wfont + x0);
	xv = (float)(17 * wfont + x0);
	y = (float)(y0 - 3.5 * hfont);
	char str[12];
	float ystep = (float)(1.5 * hfont);
	float clr;
	for(int i = 0; i < 4; i++){
		if(i == (int)m_op){
			clr = 1.0;
		}else{
			clr = 0.5;
		}

		drawGlText(x, y, items[i], 0, clr, 0, 1, GLUT_BITMAP_8_BY_13);

		switch(i){
		case EMO_EDT_RT:
			{
				ch_wp * pwp = get_ch_wp();
				if(pwp && pwp->get_num_wps() && pwp->get_num_wps() != pwp->get_focus()){
				  snprintf(str, 12, "WP[%03d]", pwp->get_focus());
				}else{
					snprintf(str, 12, "N/A");
				}
			}
			break;
		case EMO_SV_RT:
			snprintf(str, 12, "RT[%03d]", m_rt_sv);
			break;
		case EMO_LD_RT:
			snprintf(str, 12, "RT[%03d]", m_rt_ld);
			break;
		case EMO_RNG:
			snprintf(str, 12, "%4.1fkm", m_map_range * 0.001);
			break;
		}
		drawGlText(xv, y, str, 0, clr, 0, 1, GLUT_BITMAP_8_BY_13);
		y -= ystep;
	}

	y = (float)(y0 - 10. * hfont);
	drawGlText(x, y, mexp[m_op], 0, 1, 0, 1, GLUT_BITMAP_8_BY_13);
}

void c_aws1_ui_map::key(int key, int scancode, int action, int mods)
{
}

void c_aws1_ui_map::draw_ship_object(const float x, const float y, const float z, 
		const float vx, const float vy, const float vz, const float yw,
		const float tcpa, const float dcpa,
		const float xp, const float yp, const float sp)
{
	float theta = (float)(yw * (PI / 180.));
	float c = (float) cos(theta), s = (float) sin(theta);

	float ws, hs, wf, hf;
	pix2nml(10., 10., ws, hs);
	pix2nml(8, 13, wf, hf);
	Point2f pt0 = offset;
	pt0.x += x * ifxmeter;
	pt0.y += y * ifymeter;

	Point2f pts[3];
	for(int i = 0; i < 3; i++){
		// rotating the points
		// multiply [ c s; -s c]
		pts[i].x = (float)(ws * (m_ship_pts[i].x * c + m_ship_pts[i].y * s));
		pts[i].y = (float)(hs * (- m_ship_pts[i].x * s + m_ship_pts[i].y * c));
	}
	drawGlPolygon2Df(pts, 3, pt0, 0, 1, 0, 0.5); // own ship triangle
	float lw;
	pix2nml(1, 1, lw, lw);
	drawGlLine2Df(pt0.x, pt0.y, 
		(float)(pt0.x + vx * ifxmeter * 180.f), (float)(pt0.y + vy * ifymeter * 180.f), 
		0., 1., 0., 1.0, lw);

	if (tcpa != 0){
		char str[128];
		Point2f pt1;
		snprintf(str, 128, "T%3.1f D%3.1f", tcpa, dcpa);
		int l = strlen(str) + 1;
		pt1.x = pt0.x + wf;
		pt1.y = pt0.y + hf;

		// drawing tcpa/dcpa
		drawGlLine2Df(pt0.x, pt0.y, pt1.x, pt1.y, 0, 1, 0, 1, 1);
		drawGlLine2Df(pt1.x, pt1.y, pt1.x + l * wf, pt1.y, 0, 1, 0, 1, 1);
		drawGlText(pt1.x + wf, pt0.y + 1.5 * hf, str, 0, 1, 0, 1, GLUT_BITMAP_8_BY_13);

		pt1 = offset;
		pt1.x += xp * ifxmeter;
		pt1.y += yp * ifymeter;
		draw_circle(pt1, sp, 0, 1, 0, 1, lw);
		drawGlLine2Df(pt0.x, pt0.y, pt1.x, pt1.y, 0, 1, 0, 1, lw);
	}
}


void c_aws1_ui_map::draw_circle(const Point2f & pt, const float s, 
	const float r, const float g, const float b, const float a, const float lw)
{
	Point2f pts[36];
	for (int i = 0; i < 36; i++){
		pix2nml(
			(float)(m_circ_pts[i].x * s),
			(float)(m_circ_pts[i].y * s),
			pts[i].x, pts[i].y);
	}
	drawGlPolygon2Df(pts, 36, pt, r, g, b, a, lw);
}

