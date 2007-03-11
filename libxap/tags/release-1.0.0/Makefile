#**************************************************************************
#
#	Makefile.in
#
#	libxap makefile
#	
#	Copyright (c) 2007 Daniel Berenguer <dberenguer@usapiens.com>
#	
#	This file is part of the OPNODE project.
#	
#	OPNODE is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2 of the License, or
#	(at your option) any later version.
#	
#	OPNODE is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#	GNU General Public License for more details.
#	
#	You should have received a copy of the GNU General Public License
#	along with OPNODE; if not, write to the Free Software
#	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#	
#	
#	Last changes:
#	
#	03/09/07 by Daniel Berenguer : first version.
#
#***************************************************************************/

.PHONY: bin debug

all: bin debug copy_files

host:
	$(MAKE) -C $@
bin:
	$(MAKE) -C $@
debug:
	$(MAKE) -C $@

clean:
	$(MAKE) -C bin $@
	$(MAKE) -C debug $@
	$(MAKE) -C host $@

copy_files:
	cp bin/libxap.a /usr/local/LxNETES-2.3/arm-elf/uClibc-0.9.19/lib/
	cp src/globalxap.h /usr/local/LxNETES-2.3/arm-elf/uClibc-0.9.19/include/opnode/
