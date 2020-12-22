#include "geos_spat.h"


SpatVector SpatVector::allerretour() {
	GEOSContextHandle_t hGEOSCtxt = geos_init();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	SpatVectorCollection out = vect_from_geos(g, hGEOSCtxt, type());
	geos_finish(hGEOSCtxt);
	return out.get(0);
}

SpatVectorCollection SpatVector::bienvenue() {
	GEOSContextHandle_t hGEOSCtxt = geos_init();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	SpatVectorCollection out = coll_from_geos(g, hGEOSCtxt);
	geos_finish(hGEOSCtxt);
	return out;
}


std::vector<bool> SpatVector::geos_isvalid() {
	GEOSContextHandle_t hGEOSCtxt = geos_init2();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	std::vector<bool> out;
	out.reserve(g.size());
	for (size_t i = 0; i < g.size(); i++) {		
		char v = GEOSisValid_r(hGEOSCtxt, g[i].get());
		out.push_back(v);
	}
	geos_finish(hGEOSCtxt);
	return {out};
}

std::vector<std::string> SpatVector::geos_isvalid_msg() {
	GEOSContextHandle_t hGEOSCtxt = geos_init2();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	std::vector<bool> ok;
	ok.reserve(g.size());
	for (size_t i = 0; i < g.size(); i++) {		
		char v = GEOSisValid_r(hGEOSCtxt, g[i].get());
		ok.push_back(v);
	}
	geos_finish(hGEOSCtxt);
	std::vector<std::string> out;

	size_t j=0;

	for (size_t i=0; i<ok.size(); i++) {
		if (!ok[i]) {
			if (j < msgs.size()) {
				out.push_back(msgs[j]);
				j++;
			} else {
				out.push_back("?");	
			}
		}
	}
    msgs.resize(0); 
	return out;
}
  

SpatVector SpatVector::buffer2(double dist, unsigned nQuadSegs, unsigned capstyle) {

	std::string vt = type();
	if ((vt == "points") && (dist <= 0)) {
		dist = -dist;
	}

	GEOSContextHandle_t hGEOSCtxt = geos_init();
	SpatVector out;
	SpatVector f = remove_holes();

	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	std::vector<GeomPtr> b(size());
	for (size_t i = 0; i < g.size(); i++) {
		GEOSGeometry* pt = GEOSBuffer_r(hGEOSCtxt, g[i].get(), dist, nQuadSegs);
		if (pt == NULL) {
			out.setError("GEOS exception");
			geos_finish(hGEOSCtxt);
			return(out);
		} 
		b[i] = geos_ptr(pt, hGEOSCtxt);	
	}
	SpatVectorCollection coll = vect_from_geos(b, hGEOSCtxt, "polygons");

//	out = spat_from_geom(hGEOSCtxt, g, "points");
	geos_finish(hGEOSCtxt);
	return coll.get(0);	
}


SpatVector SpatVector::intersect(SpatVector v) {

	SpatVector out;

	GEOSContextHandle_t hGEOSCtxt = geos_init();
	std::vector<GeomPtr> x = geos_geoms(this, hGEOSCtxt);
	std::vector<GeomPtr> y = geos_geoms(&v, hGEOSCtxt);	
	std::vector<GeomPtr> result;
	std::vector<std::vector<unsigned>> atts(2);
	size_t nx = size();
	size_t ny = v.size();
	
		
	for (size_t i = 0; i < nx; i++) {
		for (size_t j = 0; j < ny; j++) {
			GEOSGeometry* geom = GEOSIntersection_r(hGEOSCtxt, x[i].get(), y[j].get());
			if (geom == NULL) {
				out.setError("GEOS exception");
				geos_finish(hGEOSCtxt);
				return(out);
			} 
			unsigned check = 0; //GEOSisEmpty_r(hGEOSCtxt, geom);
			if (check > 1) {
				out.setError("GEOS exception");
				geos_finish(hGEOSCtxt);
				return(out);			
			} else if (check == 1) {
				result.push_back(geos_ptr(geom, hGEOSCtxt));
				atts[0].push_back(i);
				atts[1].push_back(j);
			}
		}
	}

	// this should happen in the loop. And different types are possible!
	SpatVectorCollection coll = vect_from_geos(result, hGEOSCtxt, type());
	// deal with attributes
	geos_finish(hGEOSCtxt);
	return coll.get(0);	
}




SpatVector SpatVector::centroid() {

	SpatVector out;

	GEOSContextHandle_t hGEOSCtxt = geos_init();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	std::vector<GeomPtr> b(size());
	for (size_t i = 0; i < g.size(); i++) {		
		GEOSGeometry* pt = GEOSGetCentroid_r(hGEOSCtxt, g[i].get());
		if (pt == NULL) {
			out.setError("NULL geom");
			geos_finish(hGEOSCtxt);
			return out;
		}
		b[i] = geos_ptr(pt, hGEOSCtxt);	
	}
	SpatVectorCollection coll = vect_from_geos(b, hGEOSCtxt, "points");
	geos_finish(hGEOSCtxt);

	out = coll.get(0);
	out.df = df;
	return out;	
}

SpatVector SpatVector::unaryunion() {
	GEOSContextHandle_t hGEOSCtxt = geos_init();
	std::vector<GeomPtr> g = geos_geoms(this, hGEOSCtxt);
	std::vector<GeomPtr> gout(g.size());
	SpatVector out;
	for (size_t i = 0; i < g.size(); i++) {		
		GEOSGeometry* u = GEOSUnaryUnion_r(hGEOSCtxt, g[i].get());
		if (u == NULL) {
			out.setError("NULL geom");
			geos_finish(hGEOSCtxt);
			return out;
		}
		gout[i] = geos_ptr(u, hGEOSCtxt);
	}
	SpatVectorCollection coll = coll_from_geos(gout, hGEOSCtxt);
	geos_finish(hGEOSCtxt);
	out = coll.get(0);
	out.srs = srs;
	return out;
}

/*
bool geos_buffer(GEOSContextHandle_t hGEOSCtxt, std::vector<GeomPtr> &g, double dist, unsigned nQuadSegs) {
	std::vector<GeomPtr> g(size());
	for (size_t i = 0; i < g.size(); i++) {
		GEOSGeometry* pt = GEOSBuffer_r(hGEOSCtxt, g[i].get(), dist, nQuadSegs);
		if (pt == NULL) {
			return false;
		} 
		g[i] = geos_ptr(pt, hGEOSCtxt);	
	}
	return true;
}
*/

