#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	NewConfigWindow newwin;

	union
	{
		struct
		{
			struct TagItem palette_tags[4];
			ObjectDef object_def[4];
		} stuff1;

		struct
		{
			char gun_bits[3];
			char pad;
			GL_Object *palette_gadget[2];
		} stuff2;
	} stuff;

	IPCData *owner_ipc;

	/*struct Library			*DOpusBase;
	struct Library			*IntuitionBase;*/
	ULONG pad2[2];

	unsigned long flags;

	DOpusScreenData screen_data;
	short pen_array[26];

	unsigned char fgpen;
	unsigned char bgpen;

	IPCData *main_ipc;

	short pen_count;
	short pad;

	struct TextAttr *font;
	struct Window *window;
	ObjectList *list;

	ULONG pad1[2];
} PaletteBoxData;
#ifndef __amigaos3__
	#pragma pack()
#endif

#define PBF_NO_FG (1 << 0)
#define PBF_NO_EDIT (1 << 1)
#define PBF_DO_FONT (1 << 2)

extern ConfigWindow _palette_box_window, _palette_box_cust_window;
extern struct TagItem _palette_box_tags[];
extern ObjectDef _palette_box_objects[], _palette_box_cust_objects[];

void SAVEDS PaletteBox(void);
