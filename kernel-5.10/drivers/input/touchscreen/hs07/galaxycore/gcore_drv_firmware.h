/*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#ifndef _GCORE_FIRMWARE_H_
#define _GCORE_FIRMWARE_H_

#ifndef CONFIG_UPDATE_FIRMWARE_BY_BIN_FILE

unsigned char gcore_default_FW[] = {


};

#if defined(CONFIG_GCORE_AUTO_UPDATE_FW_FLASHDOWNLOAD)
unsigned char gcore_flash_op_FW[] = {


};
#endif

#endif /* CONFIG_UPDATE_FIRMWARE_BY_BIN_FILE */

unsigned char gcore_mp_FW[] = {


};

#endif /* _GCORE_FIRMWARE_H_ */
