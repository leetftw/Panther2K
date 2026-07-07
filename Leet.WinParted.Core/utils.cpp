#include "utils.h"

namespace Leet
{
	namespace WinParted
	{
		namespace Utils
		{
			long CalculateCRC32(char* data, unsigned long long len)
			{
				// Code ripped and modified from here:
				// https://lxp32.github.io/docs/a-simple-example-crc32-calculation/
				// Generate a CRC table with the CRC32 polynomial

				/*long crc = 0xFFFFFFFF;
				for (size_t i = 0; i < len; i++) {
					char ch = data[i];
					for (long j = 0; j < 8; j++) {
						long b = (ch ^ crc) & 1;
						crc >>= 1;
						if (b) crc = crc ^ 0xEDB88320;
						ch >>= 1;
					}
				}
				return crc;*/

				// Code ripped from here
				// gdisk

				unsigned long table[256];
				unsigned long polynomial = 0xEDB88320;
				for (int i = 0; i < 256; i++)
				{
					unsigned long c = i;
					for (int j = 0; j < 8; j++)
					{
						if (c & 1) {
							c = polynomial ^ (c >> 1);
						}
						else {
							c >>= 1;
						}
					}
					table[i] = c;
				}

				// Calculate the CRC
				register unsigned long c = 0xFFFFFFFF;
				for (unsigned long i = 0; i < len; i++)
				{
					c = ((c >> 8) & 0x00FFFFFF) ^ table[(c ^ *data++) & 0xFF];
				}

				return c ^ 0xFFFFFFFF;
			}
		}
	}
}