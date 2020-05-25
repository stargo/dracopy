/* -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil -*-
 * vi: set shiftwidth=2 tabstop=2 expandtab:
 * :indentSize=2:tabSize=2:noTabs=true:
 */

#include "ops.h"
#include "screen.h"
#include "version.h"
#include "base.h"
#include <cbm.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

const char *value2hex = "0123456789abcdef";

#ifdef NOCOLOR
const BYTE textc = COLOR_WHITE;
#else
const BYTE textc = COLOR_LIGHTGREEN;
#endif

extern const BYTE textc;
Directory* dirs[] = {NULL,NULL};
BYTE devices[] = {8,9};
char linebuffer[SCREENW+1];
char linebuffer2[SCREENW+1];
char answer[40];

BYTE DIR1H;
BYTE DIR2H;

void
initDirWindowHeight()
{
#ifdef CHAR80
  DIR1H = 23;
  DIR2H = 23;
#else
  DIR1H = 11;
  DIR2H = 10;
#endif
}

char DOSstatus[40];

/// string descriptions of enum drive_e
const char* drivetype[LAST_DRIVE_E] = {"\0", "1540", "1541", "1551", "1570", "1571", "1581", "sd2iec", "cmd", "vice"};
/// enum drive_e value for each device 0-11.
BYTE devicetype[12];

const char*
getDeviceType(BYTE context)
{
  const BYTE device = devices[context];
  BYTE idx;
  if (device > sizeof(devicetype))
    {
      return "!d";
    }
  idx = cmd(device, "ui");
  if (idx != 73)
    {
      linebuffer2[0] = 'Q';
      linebuffer2[1] = value2hex[idx >> 4];
      linebuffer2[2] = value2hex[idx & 15];
      linebuffer2[3] = 0;
      return linebuffer2;
    }
  for(idx = 1; idx < LAST_DRIVE_E; ++idx)
    {
      if(strstr(DOSstatus, drivetype[idx]))
        {
          devicetype[device] = idx;
          return drivetype[idx];
        }
    }
  if(strstr(DOSstatus, "tdisk"))
    {
      devicetype[device] = D1551;
      return drivetype[D1551];
    }
  return "!nf";
}

BYTE
dosCommand(const BYTE lfn, const BYTE drive, const BYTE sec_addr, const char *cmd)
{
  int res;
	if (cbm_open(lfn, drive, sec_addr, cmd) != 0)
    {
      return _oserror;
    }

	if (lfn != 15)
    {
      if (cbm_open(15, drive, 15, "") != 0)
        {
          cbm_close(lfn);
          return _oserror;
        }
    }

	DOSstatus[0] = 0;
	res = cbm_read(15, DOSstatus, sizeof(DOSstatus));

	if(lfn != 15)
    {
      cbm_close(15);
    }
	cbm_close(lfn);

  if (res < 1)
    {
      return _oserror;
    }

#if 0
  gotoxy(0,BOTTOM);
  cputs(DOSstatus);
#endif
	return (DOSstatus[0] - 48) * 10 + DOSstatus[1] - 48;
}

int
cmd(unsigned char device, const char *cmd)
{
  return dosCommand(15, device, 15, cmd);
}

void
execute(char * prg, BYTE device)
{
  // prepare the screen with the basic command to load the next program
	exitScreen();
	gotoxy(0,2);
	cprintf("load\"%s\",%d\n\r\n\r\n\r\n\r\n\r",prg,device);
	cputs("run\n\r");
  gotoxy(14,BOTTOM-1);
  cputs("this program was loaded by");
  gotoxy(14,BOTTOM);
  cputs("DraCopy " DRA_VER);

  // put two CR in keyboard buffer
	*((unsigned char *)KBCHARS)=13;
	*((unsigned char *)KBCHARS+1)=13;
	*((unsigned char *)KBNUM)=2;

  // exit DraCopy, which will execute the BASIC LOAD above
	gotoxy(0,0);
  exit(0);
}

void
updateScreen(const BYTE context, BYTE num_dirs)
{
#ifdef CHAR80
  clrscr();
#endif
	updateMenu();
	showDir(context, dirs[context], context);
  if (num_dirs > 1)
    {
      const BYTE other_context = context^1;
      showDir(context, dirs[other_context], other_context);
    }
}

int
main(void)
{
  initScreen(COLOR_BLACK, COLOR_BLACK, textc);
  mainLoop();
  exitScreen();
  return 0;
}

void
about(const char *progname)
{
	BYTE idx=0;
	newscreen("About");
	textcolor(COLOR_YELLOW);
	idx=4;
	gotoxy(0,idx++);
  cputs(progname);
	cputs(" " DRA_VER);
#ifdef CHAR80
	cputs(" 80 Chars");
#endif
	textcolor(COLOR_GREEN);
	idx++;
	idx++;
	gotoxy(0,idx++);
	cputs("Copyright 2009 by Draco and others");
	idx++;
	gotoxy(0,idx++);
	cputs("https://github.com/doj/dracopy");
	idx++;
	idx++;
	gotoxy(0,idx++);
	cputs("THIS PROGRAM IS DISTRIBUTED IN THE HOPE");
	gotoxy(0,idx++);
	cputs("THAT IT WILL BE USEFUL.");
	idx++;
	gotoxy(0,idx++);
	cputs("IT IS PROVIDED WITH NO WARRANTY OF ANY ");
	gotoxy(0,idx++);
	cputs("KIND.");
	idx++;
	gotoxy(0,idx++);
	textcolor(COLOR_LIGHTRED);
	cputs("USE IT AT YOUR OWN RISK!");
	gotoxy(0,24);
	textcolor(COLOR_VIOLET);
	waitKey(0);
}

void
clrDir(BYTE context)
{
	clearArea(DIRX+1, DIRY+1, DIRW, DIRH);
}

void
refreshDir(const BYTE context)
{
	Directory * cwd = NULL;
  textcolor(COLOR_WHITE);
	cwd = readDir(cwd, devices[context], context);
	dirs[context]=cwd;
	cwd->selected=cwd->firstelement;
	showDir(context, cwd, context);
#if 0
	if (devices[0]==devices[1])
    {
      // refresh also other dir if it's the same drive
      const BYTE other_context = context^1;
      dirs[other_context] = readDir(dirs[other_context], devices[other_context], other_context);
      showDir(context, cwd, other_context);
    }
#endif
}

const char *reg_types[] = { "SEQ","PRG","URS","REL","VRP" };
const char *oth_types[] = { "DEL","CBM","DIR","LNK","OTH","HDR"};
char bad_type[4];
const char*
fileTypeToStr(BYTE ft)
{
  if (ft & _CBM_T_REG)
    {
      ft &= ~_CBM_T_REG;
      if (ft <= 4)
        return reg_types[ft];
    }
  else
    {
      if (ft <= 5)
        return oth_types[ft];
    }
  bad_type[0] = '?';
  bad_type[1] = value2hex[ft >> 4];
  bad_type[2] = value2hex[ft & 15];
  bad_type[3] = 0;
  return bad_type;
}

static void
printElementPriv(const BYTE context, const Directory *dir, const DirElement *current, const BYTE xpos, const BYTE ypos)
{
	Directory * cwd = GETCWD;
  gotoxy(xpos,ypos);
  if ((current == dir->selected) && (cwd == dir))
    {
      revers(1);
    }

  cprintf("%4d %-16s %s", current->dirent.size, current->dirent.name, fileTypeToStr(current->dirent.type));

  if (current->flags!=0)
    {
      gotoxy(xpos,ypos);
      textcolor(COLOR_WHITE);
      cputc('>');
    }

  textcolor(textc);
  revers(0);
}

void
printDir(const BYTE context, const Directory *dir, const BYTE xpos, const BYTE ypos)
{
	DirElement * current;
	int selidx = 0;
	int page = 0;
	int skip = 0;
	int pos = 0;
	int idx = 0;
  const char *typestr = NULL;

	if (dir==NULL)
    {
      clrDir(context);
      return;
    }

  revers(0);
  current = dir->firstelement;
  idx=0;
  while (current!=NULL)
    {
      if (current==dir->selected)
        {
          break;
        }
      idx++;
      current=current->next;
    }

  page=idx/DIRH;
  skip=page*DIRH;

  current = dir->firstelement;

  // skip pages
  if (page>0)
    {
      for (idx=0; (idx < skip) && (current != NULL); ++idx)
        {
          current=current->next;
          pos++;
        }
    }

  for(idx=0; (current != NULL) && (idx < DIRH); ++idx)
    {
      printElementPriv(context, dir, current, xpos, ypos+idx+1);
      current = current->next;
    }

  // clear empty lines
  for (;idx < DIRH; ++idx)
    {
      gotoxy(xpos,ypos+idx+1);
      cputs("                         ");
    }
}

void
printElement(const BYTE context, const Directory *dir, const BYTE xpos, const BYTE ypos)
{
	const DirElement *current;

	int page = 0;
	int idx = 0;
	int pos = 0;
	int yoff=0;

	if (dir==NULL || dir->firstelement == NULL)
    {
      return;
    }

  revers(0);
  current = dir->firstelement;

  pos = dir->pos;

  idx=pos;
  while (current!=NULL && (idx--) >0)
    {
      current=current->next;
    }

  page=pos/DIRH;
  yoff=pos-(page*DIRH);

  printElementPriv(context, dir, current, xpos, ypos+yoff+1);
}

void
drawDirFrame(BYTE context, const Directory *dir, const BYTE mycontext)
{
  sprintf(linebuffer, " %02i:%s", (int)devices[mycontext], dir ? dir->name : "");
	if(mycontext==context)
    {
      linebuffer[0] = '>';
      textcolor(COLOR_WHITE);
    }
	else
    {
      textcolor(textc);
    }

  if (dir)
    {
      sprintf(linebuffer2, "%s>%u bl free<", dir->device_type, dir->free);
    }
  else
    {
      linebuffer2[0] = 0;
    }
  context = mycontext; // set context so that DIRX and friends below work.
	drawFrame(linebuffer, DIRX, DIRY, DIRW+2, DIRH+2, linebuffer2);
	textcolor(textc);
}

void
showDir(BYTE context, const Directory * dir, const BYTE mycontext)
{
  drawDirFrame(context, dir, mycontext);
  context = mycontext;
	printDir(context, dir, DIRX+1, DIRY);
}

void
changeDir(const BYTE context, const BYTE device, const char *dirname)
{
  if (dirname)
    {
      BYTE mount = 0;
      register BYTE l = strlen(dirname);
      if (l > 4 && dirname[l-4] == '.')
        {
          if ((dirname[l-3] == 'd' || dirname[l-3] == 'D') &&
              (dirname[l-2] == '6') &&
              (dirname[l-1] == '4'))
            {
              mount = 1;
            }
          else if ((dirname[l-3] == 'd' || dirname[l-3] == 'D') &&
                   (dirname[l-2] == '7' || dirname[l-2] == '8') &&
                   (dirname[l-1] == '1'))
            {
              mount = 1;
            }
        }
      if (mount ||
          (l == 1 && dirname[0]==CH_LARROW))
        {
          sprintf(linebuffer, "cd:%s", dirname);
        }
      else
        {
          sprintf(linebuffer, "cd/%s/", dirname);
        }
    }
  else
    {
      strcpy(linebuffer, "cd//");
    }
  cmd(device, linebuffer);
  refreshDir(context);
}

void
changeDeviceID(BYTE device)
{
  int i;
  newscreen(" change device ID");
  cprintf("\n\rchange device ID %i to (0-255): ", device);
  sprintf(linebuffer, "%i", device);
  i = textInput(31, 2, linebuffer, 3);
  if (i <= 0)
    return;
  i = atoi(linebuffer);

  if (devicetype[device] == SD2IEC)
    {
      sprintf(linebuffer, "U0>%c", i);
    }
  else
    {
      // TODO: doesn't work

      // Commodore drives:
      // OPEN 15,8,15:PRINT#15,"M-W";CHR$(119);CHR$(0);CHR$(2);CHR$(device number+32);CHR$(device number+64):CLOSE 15
      char *s = linebuffer;
      *s++ = 'm';
      *s++ = '-';
      *s++ = 'w';
      *s++ = 119; // addr lo
      *s++ = 0;   // addr hi
      *s++ = 2;   // number of bytes
      *s++ = 32+i;// device num + 0x20 for LISTEN
      *s++ = 64+i;// device num + 0x40 for TALK
      *s = 0;
    }

  cmd(device, linebuffer);
}

void
debugs(const char *s)
{
  gotoxy(30,BOTTOM);
  cclear(10);
  gotoxy(30,BOTTOM);
  cputs(s);
}

void
debugu(const unsigned u)
{
  gotoxy(30,BOTTOM);
  cclear(10);
  gotoxy(30,BOTTOM);
  cprintf("%04x", u);
}

/**
 * input/modify a string.
 * based on version 1.0e, then modified.
 * @param[in] xpos screen x where input starts.
 * @param[in] ypos screen y where input starts.
 * @param[in,out] str string that is edited, it can have content and must have at least @p size + 1 bytes. Maximum size if 255 bytes.
 * @param[in] size maximum length of @p str in bytes.
 * @return -1 if input was aborted.
 * @return >= 0 length of edited string @p str.
 */
int
textInput(const BYTE xpos, const BYTE ypos, char *str, const BYTE size)
{
	register BYTE idx = strlen(str);
	register BYTE c;

	cursor(1);
	cputsxy(xpos, ypos, str);

	while(1)
    {
      c = cgetc();
      switch (c)
        {
      case CH_ESC:
        cursor(0);
        return -1;

      case CH_ENTER:
        idx = strlen(str);
        str[idx] = 0;
        cursor(0);
        return idx;

      case CH_DEL:
        if (idx)
          {
            --idx;
            cputcxy(xpos + idx, ypos, ' ');
            for(c = idx; 1; ++c)
              {
                const BYTE b = str[c+1];
                str[c] = b;
                cputcxy(xpos + c, ypos, b ? b : ' ');
                if (b == 0)
                  break;
              }
            gotoxy(xpos + idx, ypos);
          }
        break;

        case CH_INS:
          c = strlen(str);
          if (c < size &&
              c > 0 &&
              idx < c)
            {
              ++c;
              while(c >= idx)
                {
                  str[c+1] = str[c];
                  if (c == 0)
                    break;
                  --c;
                }
              str[idx] = ' ';
              cputsxy(xpos, ypos, str);
              gotoxy(xpos + idx, ypos);
            }
          break;

      case CH_CURS_LEFT:
        if (idx)
          {
            --idx;
            gotoxy(xpos + idx, ypos);
          }
        break;

      case CH_CURS_RIGHT:
        if (idx < strlen(str) &&
            idx < size)
          {
            ++idx;
            gotoxy(xpos + idx, ypos);
          }
        break;

      default:
        if (isprint(c) &&
            idx < size)
          {
            const BYTE flag = (str[idx] == 0);
            str[idx] = c;
            cputc(c);
            ++idx;
            if (flag)
              str[idx+1] = 0;
            break;
          }
        break;
      }
    }
  return 0;
}
