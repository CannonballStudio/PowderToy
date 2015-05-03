/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "element.h"

int update_TTAN(UPDATE_FUNC_ARGS) {
	int nx, ny, ttan = 0;
	if(nt<=2)
		ttan = 2;
	else if(parts[i].tmp)
		ttan = 2;
	else if(nt<=6)
		for (nx=-1; nx<2; nx++) {
			for (ny=-1; ny<2; ny++) {
				if ((!nx != !ny) && x+nx>=0 && y+ny>=0 && x+nx<XRES && y+ny<YRES) {
					if((pmap[y+ny][x+nx]&0xFF)==PT_TTAN)
						ttan++;
				}
			}
		}
		
	if(ttan>=2) {
		bmap_blockair[y/CELL][x/CELL] = 1;
		bmap_blockairh[y/CELL][x/CELL] = 1;
	}
	return 0;
}
