#ifndef _AWS_MAP_H_
#define _AWS_MAP_H_
// Copyright(c) 2017 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// aws_map.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// aws_map.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with aws_map.h.  If not, see <http://www.gnu.org/licenses/>. 

namespace AWSMap{
class Node;
class Lv;
class c_map;

enum LayerCode
{
	 amlc_coast_line = 0, amlc_undef
};

inline bool isNewNodeIndex(int inode, list<int> & l1, list<int> & l2)
{
	return (find(l1.begin(), l1.end(), inode) != l1.end()) 
		&& (find(l2.begin(), l2.end(), inode) != l2.end());
}

extern const char * LayerStr[amlc_undef];

class c_layer
{
protected:
	bool bupdate;
public:
	c_layer();
	virtual ~c_layer();

	void setUpdate(const bool update = true)
	{
		bupdate = update;	
	}

	bool isUpdate()
	{
		return bupdate;
	}

	// save and load interface for layer data
	virtual bool save(const char * fname) = 0;
	virtual bool load(const char * fname) = 0;
	virtual LayerCode getCode() = 0;

	static c_layer * create(const LayerCode code);

	virtual void build(list<c_layer*> & ll, double spix) = 0;
	virtual unsigned int size() = 0;
};

class c_coast_line: public c_layer
{
protected:
	struct s_line{
		bool bclosed;
		vector<Point3f> points;
		s_line():bclosed(false){}
	};

	vector<s_line> lines;

	bool addLine(list<Point2f> & line);
	bool addLine(s_line & line);
public:
	c_coast_line();
	virtual ~c_coast_line();

	virtual bool save(const char * fname);
	virtual bool load(const char * fname);

	virtual LayerCode getCode()
	{
		return amlc_coast_line;
	}

	static c_coast_line * convertFromJPGIS(const char * fname);

	bool distribute(Lv & ml);
	virtual void build(list<c_layer*> & ll, double spix);
	const vector<Point3f> * getLine(int iline)
	{
		if (iline < lines.size())
			return &(lines[iline].points);
		return NULL;
	}

	virtual unsigned int size()
	{
		unsigned int sz = sizeof(unsigned int); /* number of lines*/
		for (int iline = 0; iline < (int) lines.size(); iline++){
			sz += sizeof(lines[iline].bclosed); /* closed flag */
			sz += sizeof(unsigned int); /* number of points */
			sz += lines[iline].points.size() * sizeof(Point3f); /* points */
		}
		return sz;
	}
};


struct s_node;
struct s_grid;

inline int idlen(const unsigned char * id)
{
	int len = 0;
	while (*id)
	{
		len++;
		id++;
	}
	return len;
}

inline void copy_id(const unsigned char * id, unsigned char * id_dst)
{
	do{
		*id_dst = *id;
		id_dst++;
		id++;
	} while (*id);
	*id_dst = 0;
}

inline void gen_id(const unsigned char * id_up, unsigned char id_this, unsigned char * id_dist)
{
	if (id_up){
		do{
			*id_dist = *id_up;
			id_up++;
			id_dist++;
		} while (*id_up);
	}
	*id_dist = id_this;
	id_dist++;
	*id_dist = 0;
}

inline unsigned char get_last_id(const unsigned char * id)
{
	unsigned char _idl = 0;
	while (*id){
		_idl = *id;
		id++;
	}
	return _idl;
}

inline unsigned char * gen_id(int ilv, int inode, 
	vector<vector<int>> & unl, vector<vector<unsigned char>> & tidl)
{
	unsigned char * id = new unsigned char[unl.size() - ilv + 1];
	unsigned char iid = unl.size() - ilv;
	id[iid] = 0; iid--;

	while (ilv < unl.size()){
		id[iid] = tidl[ilv][inode];
		iid--;
		inode = unl[ilv][inode];
		ilv++;
	}

	return id;
}

inline void free_id(unsigned char * id)
{
	delete[] id;
}

inline bool is_eq_id(unsigned char * id1, unsigned char * id2)
{
	while (*id1 && *id2){
		if (*id1 != *id2)
			return false;

		id1++;
		id2++;
	}

	if (*id1 != *id2)
		return false;

	return true;
}

inline bool is_parent_id(unsigned char * idp, unsigned char * idc)
{
	while (*idp && *idc){
		if (*idp != *idc)
			return false;

		idp++;
		idc++;
	}

	if (*idp == 0 && *idc != 0)
		return true;

	return false;
}

inline unsigned char get_child_id(unsigned char * idp, unsigned char *idc)
{
	while (*idp && *idc){
		if (*idp != *idc)
			return 0;

		idp++;
		idc++;
	}
	return *idc;
}


inline void free_edge(unsigned char * edge)
{
	delete[] edge;
}

struct s_node{
	float x, y, z;						// ecef position
	unsigned char lv;					// scale level
	unsigned char * id;					// full index the node id sequence of top to current layer.
	unsigned char szlns;				// number of lower level nodes
	unsigned short szedge;
	unsigned char * edge;				// connected nodes, null terminated sequence of relative indices to the node connected.  
	unsigned int offset[amlc_undef];	// in pages
	unsigned int size[amlc_undef];		// in pages
	c_layer * data[amlc_undef];			// layer data
	s_node * un;						// instantiated upper level node
	map<unsigned char, s_node*> lns;	// instantiated lower level nodes

	s_node() :x(0), y(0), z(0), id(0), szlns(0), szedge(0), edge(NULL), pprev(NULL), pnext(NULL), un(NULL)
	{ 
		for (int i = 0; i < (int)amlc_undef; i++)
		{
			offset[i] = 0;
			size[i] = 0;
			data[i] = NULL;
		}
	};

	~s_node()
	{
	}

	s_node * get_child(unsigned char * _id);

	const float dist(const float _x, const float _y, const float _z)
	{
		double dx = x - _x;
		double dy = y - _y;
		double dz = z - _z;

		return (float)sqrt(dx * dx + dy * dy + dz * dz);;
	}

	bool init(const unsigned char * _id, const unsigned char _lv);
	bool load(ifstream & fgrp, unsigned char * _id, const unsigned char _lv);
	bool save(ofstream & fgrp, unsigned short sz_edge_max);
	bool save_grph(ofstream & fgrp, unsigned short sz_edge_max);
	bool load_grph(ifstream & fgrp, vector<unsigned int *> & grph, vector<unsigned int*> & grph_idx, unsigned short & sz_edge_max);
	bool init_grph(vector<list<unsigned int>> & grph);

	bool load(ifstream & fdat, const unsigned char * _id, const unsigned char _lv, const LayerCode ly = amlc_undef);
	bool save(ofstream & fdat, const unsigned char * _id, const unsigned char _lv, const LayerCode code);
	unsigned int get_sz_graph_data(unsigned int sz_edge_max);
	void _release();
	void release();
	unsigned int count_child_nodes();

	bool append(s_node * pn);
	s_node * pprev;					// used in memory management
	s_node * pnext;					// used in memory management

	static unsigned int szpool;
	static s_node * pchank, * pool;
	static s_node * phead, * ptail; // LRU list
	static bool init(const unsigned int _szpool);
	static s_node * alloc();
	static void free(s_node * pn);
	static void used(s_node * pn);
	static s_node * disc();
};

struct s_grid
{
	int lv;
	int nNodes;
	double latStep;
	vector<double> lat;
	vector<double> lonStep;
	vector<vector<double>> lon;
	vector<vector<int>> grid;
	vector<vector<Point3f>> ecef;

	bool init(const double _step, const int _ilevel, bool becef = false);


	int get_near_lat_index(const float _lat)
	{
		int i = (int)((0.5 * PI - _lat) / latStep);
		return i;
	}

	int get_near_lon_index(const int ilat, const float _lon)
	{
		int i = (int)(_lon / lonStep[ilat]);
		return i;
	}

	const float dist2(const int ilat, const int ilon, const float x, const float y, const float z)
	{
		Point3f & p = ecef[ilat][ilon];
		float dx = (float)(p.x - x);
		float dy = (float)(p.y - y);
		float dz = (float)(p.z - z);
		return dx * dx + dy * dy + dz * dz;
	}

	const int get_near_index(const float lat, const float lon, const float x, const float y, const float z)
	{
		int ilat = get_near_lat_index(lat);
		int ilon = get_near_lon_index(ilat, lon);
		int ilat_min = ilat, ilon_min = ilon;

		float dmin = dist2(ilat, ilon, x, y, z);
		ilon++;
		float d = dist2(ilat, ilon + 1, x, y, z);
		if (d < dmin){
			dmin = d;
			ilat_min = ilat;
			ilon_min = ilon + 1;
		}

		ilat++;
		ilon = get_near_lon_index(ilat, lon);
		d = dist2(ilat, ilon, x, y, z);
		if (d < dmin){
			dmin = d;
			ilat_min = ilat;
			ilon_min = ilon;
		}

		ilon++;
		d = dist2(ilat, ilon + 1, x, y, z);
		if (d < dmin){
			dmin = d;
			ilat_min = ilat;
			ilon_min = ilon;
		}
		return grid[ilat_min][ilon_min];
	}
};

class c_map
{
private:
	int nLevels;
	double minMeterPerPix;
	double minStep;
	vector<double> steps;
	vector<double> pixels;

	double scale, iscale;
	s_grid cgrid;
	s_node root; // root node
public:
	c_map();
	c_map(const int _nLevels, const double _minMeterPerPix, const double _minStep, const double _scale, const unsigned int _nCache);
	~c_map();

	bool init(const int _nLevels, const double _minMeterPerPix, const double _minStep, const double _scale, const unsigned int _nCache, const char * _path = NULL);
	void release();

	bool load_with_ecef(const float x, const float y, const float z);
	bool load_with_bih(const float lat, const float lon, const float alt = 0.f);

	bool addJPGIS(const char * fname);

	static char path[1024];
	unsigned int nCache;
};

#define GR 1.61803398875 // golden ratio

class c_icosahedron
{
private:
	unsigned int nv, nf, ne; // number of vertices, faces, edges

	Point3f *v; // vertex in ecef
	Point2f *q; // vertex in lat/lon
	unsigned int ** f;   // face (index)
	unsigned int ** e;   // edge (index)

	/*
	Point3f v[12];
	Point2f q[12];
	int f[20][3];
	*/
public:
	c_icosahedron();
	c_icosahedron(const c_icosahedron & icshdrn);
	~c_icosahedron();
	Point3f get_mid_point(unsigned int ie) const
	{
		return (v[e[ie][0]] + v[e[ie][1]]) * 0.5;
	}

	unsigned int get_edge(unsigned int iv0, unsigned int iv1) const
	{
		if (iv0 > iv1){
			int t = iv0;
			iv0 = iv1;
			iv1 = t;
		}
		for (unsigned int ie = 0; ie < ne; ie++)
		{
			if (e[ie][0] == iv0 && e[ie][1] == iv1)
				return ie;
		}

		return UINT_MAX;
	}

	const unsigned int get_nv() const
	{
		return nv;
	}

	const unsigned int get_nf() const
	{
		return nf;
	}

	const unsigned int get_ne() const
	{
		return ne;
	}

	Point3f * getv(){
		return v;
	}

	Point2f * getq(){
		return q;
	}

	unsigned int ** getf(){
		return f;
	}
};

};

#endif
