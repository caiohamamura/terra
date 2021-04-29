// Copyright (c) 2018-2021  Robert J. Hijmans
//
// This file is part of the "spat" library.
//
// spat is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// spat is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with spat. If not, see <http://www.gnu.org/licenses/>.

#include "spatRaster.h"


#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include "sys/types.h"
#include "sys/sysinfo.h"
#include <unistd.h>
#elif __APPLE__
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif


double availableRAM() {
//https://stackoverflow.com/questions/38490320/how-to-query-amount-of-allocated-memory-on-linux-and-osx
//https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
	double ram = 1e+9;
	// return available RAM in number of double (8 byte) cells.
	#ifdef _WIN32
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);
		ram = statex.ullAvailPhys;
	#elif __linux__
		struct sysinfo memInfo;
		sysinfo (&memInfo);
		ram = memInfo.freeram;
	#elif __APPLE__

		vm_size_t page_size;
		mach_port_t mach_port;
		mach_msg_type_number_t count;
		vm_statistics64_data_t vm_stats;

		mach_port = mach_host_self();
		count = sizeof(vm_stats) / sizeof(natural_t);
		if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
			KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO,
											(host_info64_t)&vm_stats, &count)) {
			long long free_memory = ((int64_t)vm_stats.free_count +
                               (int64_t)vm_stats.inactive_count) * (int64_t)page_size;
			ram = free_memory;
		//https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		}
	
	#endif
	return ram / 8;  // 8 bytes for each double
}



bool SpatRaster::canProcessInMemory(SpatOptions &opt) {
	if (opt.get_todisk()) return false;
	double demand = size() * opt.ncopies;
	double supply = (availableRAM()) * opt.get_memfrac();
	std::vector<double> v;
	double maxsup = v.max_size(); //for 32 bit systems
	supply = std::min(supply, maxsup);
	return (demand < supply);
}


size_t SpatRaster::chunkSize(SpatOptions &opt) {
	double n = opt.ncopies;
	double frac = opt.get_memfrac();
	double cells_in_row = ncol() * nlyr() * n;
	double rows = (availableRAM()) * frac / cells_in_row;
	//double maxrows = 10000;
	//rows = std::min(rows, maxrows);
	size_t urows = floor(rows);
	urows = std::max(urows, (size_t)opt.minrows);
	if (urows < 1) return (1);
	if (urows < nrow()){
		return(urows);
	} else {
		return (nrow());
	}
}


std::vector<double> SpatRaster::mem_needs(SpatOptions &opt) {
	//returning bytes
	unsigned n = opt.ncopies; 
	double memneed  = ncell() * (nlyr() * n);
	double memavail = availableRAM(); 
	double frac = opt.get_memfrac();
	double csize = chunkSize(opt);
	double inmem = canProcessInMemory(opt); 
	std::vector<double> out = {memneed, memavail, frac, csize, inmem} ;
	return out;
}

//BlockSize SpatRaster::getBlockSize(unsigned n, double frac, unsigned steps) {
BlockSize SpatRaster::getBlockSize( SpatOptions &opt) {

	unsigned steps = opt.get_steps();

	BlockSize bs;
	size_t cs;

	if (steps > 0) {
		if (steps > nrow()) {
			steps = nrow();
		}
		bs.n = steps;
		cs = nrow() / steps;
	} else {
		cs = chunkSize(opt);
		bs.n = std::ceil(nrow() / double(cs));
	}
	bs.row = std::vector<size_t>(bs.n);
	bs.nrows = std::vector<size_t>(bs.n, cs);
	size_t r = 0;
	for (size_t i =0; i<bs.n; i++) {
		bs.row[i] = r;
		r += cs;
	}
	bs.nrows[bs.n-1] = cs - ((bs.n * cs) - nrow());

	return bs;
}

