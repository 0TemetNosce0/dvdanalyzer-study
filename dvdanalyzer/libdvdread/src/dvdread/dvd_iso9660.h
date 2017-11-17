#ifndef DVD_ISO9660_H
#define DVD_ISO9660_H 1

#include <inttypes.h>

#include "dvd_reader.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Looks for a file on the hybrid ISO9660/mini-UDF disc/imagefile and returns
 * the block number where it begins, or 0 if it is not found.  The filename
 * should be an absolute pathname on the UDF filesystem, starting with '/'.
 * For example '/VIDEO_TS/VTS_01_1.IFO'.  On success, filesize will be set to
 * the size of the file in bytes.
 */
uint32_t ISOFindFile(dvd_reader_t *aDevice, char *aFilename, uint32_t *aFileSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DVD_ISO9660_H */