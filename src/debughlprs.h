/* LPC176x based USB MIDI Organ Console Controller
 * Copyright (C) 2012  Nick Appleton (http://www.appletonaudio.com/)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#if defined(NDEBUG) || 1

#define DEBUG_PRINT(x) (void)
#define ASSERT(x)
static inline void dump_buffer(const unsigned char *data, unsigned length) { (void)data; (void)length; }

#else

#if 0 /* NOTE: was used when testing */

#define ASSERT(x)
#define DEBUG_PRINT(x) putstr(x)

//extern int __write(int zero, const char *s, int len);

static
void
putstr(const char* str)
{
    static char buf[128];
    unsigned i;
    for (i = 0; (i < sizeof(buf) - 1) && (str[i]); i++)
    {
        buf[i] = str[i];
    }
    buf[i] = '\n';
    //__write(0, buf, i + 1);
}

static
inline
void
dump_buffer(const unsigned char *data, unsigned length)
{
    char dta[128] = "BUF: ";
    unsigned i;
    for (i = 0; i < length; i++)
    {
        dta[5 + 3 * i]  = (data[i] >> 4) & 0xf;
        dta[6 + 3 * i]  = (data[i] & 0xf);
        dta[5 + 3 * i] += (dta[5 + 3 * i] < 10) ? '0' : ('a' - 10);
        dta[6 + 3 * i] += (dta[6 + 3 * i] < 10) ? '0' : ('a' - 10);
        dta[7 + 3 * i]  = ' ';
    }
    dta[5 + 3 * i] = '\0';
    DEBUG_PRINT(dta);
}

#endif

#endif

#endif /* DEBUG_H_ */
