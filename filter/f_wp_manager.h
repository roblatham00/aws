// Copyright(c) 2016 Yohei Matsumoto, All right reserved. 

// f_wp_manager.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_wp_manager.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_wp_manager.h.  If not, see <http://www.gnu.org/licenses/>. 

#ifndef _F_WP_MANAGER_H_
#define _F_WP_MANAGER_H_

#include "f_base.h"
#include "../channel/ch_state.h"
#include "../channel/ch_wp.h"

// Description:
// The filter manages the objects, and shares objects between hosts efficiently.
// 
class f_wp_manager: public f_base
{
protected:
	ch_state * m_state;
	ch_wp * m_wp;

public:
	f_wp_manager(const char * name);
	virtual ~f_wp_manager();

	virtual bool init_run();
	virtual void destroy_run();
	virtual bool proc();
};

#endif