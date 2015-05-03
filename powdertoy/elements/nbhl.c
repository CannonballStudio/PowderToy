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

int update_NBHL(UPDATE_FUNC_ARGS) {
	if (parts[i].tmp)
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] += restrict_flt(0.001f*parts[i].tmp, 0.1f, 51.2f);
	else
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] += 0.1f;
	return 0;
}
