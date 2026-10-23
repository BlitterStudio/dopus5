#include "config_lib.h"
#include "old_config.h"

int readline(char *, int, char *, int);
void DoAssignDrive(ConfigStuff *, int, char *, char *);
void DoAssignMenu(ConfigStuff *cstuff, int men, char *name, char *func);
void DoAssignGadget(ConfigStuff *cstuff, int bk, int gad, char *name, char *func);
void DoGetDevices(ConfigStuff *cstuff);
void linkinnewfiletype(ConfigStuff *, struct dopusfiletype *);
char *getstringcopy(char *str, APTR memory);

int ReadOldConfig(char *name, ConfigStuff *cstuff)
{
	int a, pos, b, bk, gad;
	ULONG size;
	UWORD ver, mag;
	char *cbuf, *buf, *tbuf;
	struct dopusfiletype *newtype;
	struct dopushotkey *curhotkey = 0;
	GadgetBank *bank = 0, *temp;
	struct Config *config;
	BPTR in;
	BPTR lock;
	D_S(struct FileInfoBlock, fib)

	if (!(config = cstuff->config))
		return (-1);

	if (!(lock = Lock(name, ACCESS_READ)))
		return (ERROR_OBJECT_NOT_FOUND);
	Examine(lock, fib);
	UnLock(lock);
	size = fib->fib_Size - sizeof(struct Config);
	if (!(in = Open(name, MODE_OLDFILE)))
		return (IoErr());

	if ((Read(in, (char *)&ver, 2)) < 2 || (Read(in, (char *)&mag, 2)) < 2 || mag != CONFIG_MAGIC || ver == 0xde)
	{
		Close(in);
		return (ERROR_NOT_CONFIG);
	}

	Seek(in, 0, OFFSET_BEGINNING);
	if ((Read(in, (char *)config, sizeof(struct Config))) < sizeof(struct Config))
	{
		Close(in);
		return (IoErr());
	}

	if (!(cstuff->memory = NewMemHandle(0, 0, MEMF_CLEAR)))
	{
		Close(in);
		return ERROR_NO_FREE_STORE;
	}

	if (!(cbuf = (char *)AllocMemH(cstuff->memory, size)) || !(buf = (char *)AllocMemH(cstuff->memory, 4096)) ||
		!(tbuf = (char *)AllocMemH(cstuff->memory, 256)))
	{
		Close(in);
		FreeMemHandle(cstuff->memory);
		return (ERROR_NO_FREE_STORE);
	}
	Read(in, cbuf, size);
	Close(in);

	pos = 0;
	for (a = 0; a < MENUCOUNT; a++)
	{
		pos = readline(cbuf, pos, buf, size);
		if (buf[0])
			DoAssignMenu(cstuff, a, (char *)-1, buf);
		if (pos == -1)
			goto endthis;
	}
	for (a = 0; a < MENUCOUNT; a++)
	{
		pos = readline(cbuf, pos, buf, size);
		if (buf[0])
			DoAssignMenu(cstuff, a, buf, (char *)-1);
		if (pos == -1)
			goto endthis;
	}

	for (a = 0; a < DRIVECOUNT; a++)
	{
		pos = readline(cbuf, pos, buf, size);
		if (buf[0])
			DoAssignDrive(cstuff, a, (char *)-1, buf);
		if (pos == -1)
			goto endthis;
	}

	bk = -1;
	FOREVER
	{
		if (pos >= size || pos == -1)
			break;
		if (cbuf[pos] == 5 && cbuf[pos + 1] == 'H')
		{
			struct dopushotkey *hotkey;
			pos += 2;
			if ((pos + sizeof(struct dopushotkey)) >= size)
				break;
			if ((hotkey = AllocMemH(cstuff->memory, sizeof(struct dopushotkey))))
			{
				CopyMem((char *)&cbuf[pos], (char *)hotkey, sizeof(struct dopushotkey));
				hotkey->func.function = NULL;
				hotkey->next = NULL;
			}
			pos += sizeof(struct dopushotkey);
			if ((pos = readline(cbuf, pos, buf, size)) == -1)
				break;
			if (hotkey)
			{
				hotkey->func.function = getstringcopy(buf, cstuff->memory);
				if (curhotkey)
					curhotkey->next = hotkey;
				else
					cstuff->firsthotkey = hotkey;
				curhotkey = hotkey;
			}
		}
		else if (cbuf[pos] == 6 && cbuf[pos + 1] == 'F')
		{
			pos += 2;
			if ((pos + sizeof(struct wr_dopusfiletype)) >= size)
				break;
			if ((newtype = (Filetype *)AllocMemH(cstuff->memory, sizeof(Filetype))))
			{
				CopyMem((char *)&cbuf[pos], (char *)newtype, sizeof(struct wr_dopusfiletype));
				pos += sizeof(struct wr_dopusfiletype);
				newtype->recognition = NULL;
				for (a = 0; a < FILETYPE_FUNCNUM; a++)
					newtype->function[a] = NULL;
				if ((pos = readline(cbuf, pos, buf, size)) == -1)
					break;
				newtype->recognition = getstringcopy(buf, cstuff->memory);
				for (a = 0; a < FILETYPE_FUNCNUM; a++)
				{
					if ((pos = readline(cbuf, pos, buf, size)) == -1)
						break;
					newtype->function[a] = getstringcopy(buf, cstuff->memory);
				}
				newtype->iconpath = NULL;
				pos = readline(cbuf, pos, buf, size);
				if (buf[0])
					newtype->iconpath = getstringcopy(buf, cstuff->memory);
				linkinnewfiletype(cstuff, newtype);
			}
		}
		else if (cbuf[pos] == 3 && cbuf[pos + 1] == 'G')
		{
			b = sizeof(struct newdopusfunction) * GADCOUNT;
			pos += 2;
			if ((pos + b) >= size)
				break;
			++bk;
			if (!(temp = AllocMemH(cstuff->memory, sizeof(GadgetBank))))
				goto endthis;
			if (!cstuff->firstbank)
				cstuff->firstbank = temp;
			else
			{
				bank->next = temp;
				temp->prev = bank;
			}
			bank = temp;

			CopyMem((char *)&cbuf[pos], (char *)bank->gadgets, sizeof(struct newdopusfunction) * GADCOUNT);
			pos += (sizeof(struct newdopusfunction) * GADCOUNT);

			for (gad = 0; gad < GADCOUNT; gad++)
			{
				bank->gadgets[gad].function = NULL;
				buf[0] = 0;
				if (pos > -1)
				{
					pos = readline(cbuf, pos, buf, size);
					DoAssignGadget(cstuff, bk, gad, (char *)-1, buf);
				}
			}

			for (gad = 0; gad < GADCOUNT; gad++)
			{
				bank->gadgets[gad].name = NULL;
				buf[0] = 0;
				if (pos > -1)
				{
					pos = readline(cbuf, pos, buf, size);
					DoAssignGadget(cstuff, bk, gad, buf, (char *)-1);
				}
			}
			if (pos == -1)
				break;
		}
	}
endthis:
	return (1);
}

int readline(char *buf, int pos, char *buf1, int size)
{
	int a;

	for (a = 0; a < 4096; a++)
	{
		if (size == pos || buf[pos] == 0)
		{
			buf1[a] = 0;
			if (size == pos)
				return (-1);
			return (pos + 1);
		}
		buf1[a] = buf[pos];
		++pos;
	}
	buf1[pos] = 0;
	return (pos);
}

void DoAssignGadget(ConfigStuff *cstuff, int bk, int gad, char *name, char *func)
{
	GadgetBank *bank, *temp;
	int a;

	bank = cstuff->firstbank;
	for (a = 0; a < bk; a++)
	{
		if (!bank || !bank->next)
			break;
		bank = bank->next;
	}
	if (a < bk || !bank)
	{
		if (!(temp = AllocMemH(cstuff->memory, sizeof(GadgetBank))))
			return;
		if (bank)
			bank->next = temp;
		else
			cstuff->firstbank = temp;
		bank = temp;
	}

	if (name != (char *)-1)
	{
		if (name && name[0])
			bank->gadgets[gad].name = getstringcopy(name, cstuff->memory);
	}
	if (func != (char *)-1)
	{
		if (func && func[0])
			bank->gadgets[gad].function = getstringcopy(func, cstuff->memory);
	}
}

void DoAssignMenu(ConfigStuff *cstuff, int men, char *name, char *func)
{
	struct Config *config;

	if (!(config = cstuff->config))
		return;
	if (name != (char *)-1)
	{
		if (name && name[0])
			config->menu[men].name = getstringcopy(name, cstuff->memory);
	}
	if (func != (char *)-1)
	{
		if (func && func[0])
			config->menu[men].function = getstringcopy(func, cstuff->memory);
	}
}

void DoAssignDrive(ConfigStuff *cstuff, int drv, char *name, char *path)
{
	struct Config *config;

	if (!(config = cstuff->config))
		return;
	if (name != (char *)-1)
	{
		if (!name)
			config->drive[drv].name[0] = 0;
		else
			strcpy(config->drive[drv].name, name);
	}
	if (path != (char *)-1)
	{
		if (path && path[0])
			config->drive[drv].function = getstringcopy(path, cstuff->memory);
	}
}

void linkinnewfiletype(ConfigStuff *cstuff, struct dopusfiletype *temp)
{
	struct dopusfiletype *pos;

	temp->next = NULL;
	if (!(pos = cstuff->firsttype))
		cstuff->firsttype = temp;
	else
	{
		while (pos->next)
		{
			if (strcmp(pos->next->type, "Default") == 0)
			{
				temp->next = pos->next;
				break;
			}
			pos = pos->next;
		}
		pos->next = temp;
	}
}

void FreeOldConfig(ConfigStuff *cstuff)
{
	if (cstuff)
		FreeMemHandle(cstuff->memory);
}

char *getstringcopy(char *str, APTR memory)
{
	char *copy;

	if (str && (copy = AllocMemH(memory, strlen(str) + 1)))
	{
		strcpy(copy, str);
		return copy;
	}
	return 0;
}
