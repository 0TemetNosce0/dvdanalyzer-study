#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>

#include "logger.h"
#include "pgcanalyzer.h"
#include "list.h"

#ifdef QT_QML_DEBUG
#include "leak_detector_c.h"
#endif


//AriettyPomPoko.iso


#define MAX_VTS_COUNT 100

/* link command types */
typedef enum {
    LinkNoLink  = 0,

    LinkTopC    = 1,
    LinkNextC   = 2,
    LinkPrevC   = 3,

    LinkTopPG   = 5,
    LinkNextPG  = 6,
    LinkPrevPG  = 7,

    LinkTopPGC  = 9,
    LinkNextPGC = 10,
    LinkPrevPGC = 11,
    LinkGoUpPGC = 12,
    LinkTailPGC = 13,

    LinkRSM     = 16,

    LinkPGCN,
    LinkPTTN,
    LinkPGN,
    LinkCN,

    Exit,

    JumpTT, /* 22 */
    JumpVTS_TT,
    JumpVTS_PTT,

    JumpSS_FP,
    JumpSS_VMGM_MENU,
    JumpSS_VTSM,
    JumpSS_VMGM_PGC,

    CallSS_FP, /* 29 */
    CallSS_VMGM_MENU,
    CallSS_VTSM,
    CallSS_VMGM_PGC,

    PlayThis
} link_cmd_t;

/* a link's data set */
typedef struct {
  link_cmd_t command;
  uint16_t   data1;
  uint16_t   data2;
  uint16_t   data3;
} link_t;

/* 命令类型定义 */
typedef enum {
    Link_Command,
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    union {
        void *common;
        link_t *link;
    };
} cmd_t;

//命令表定义
typedef struct {
    uint32_t number_of_pre;
    cmd_t **pre_cmd;
    uint32_t number_of_post;
    cmd_t **post_cmd;
    uint32_t number_of_cell;
    cmd_t **cell_cmd;
} cmd_table_t;

static void destroy_cmd_table(cmd_table_t *cmd) {
    uint32_t i;

    if (cmd == NULL) {
        return;
    }

    if (cmd->pre_cmd && cmd->number_of_pre > 0) {
        for (i=0; i<cmd->number_of_pre; i++) {
            if (cmd->pre_cmd[i]) {
                if (cmd->pre_cmd[i]->link) {
                    free(cmd->pre_cmd[i]->link);
                    cmd->pre_cmd[i]->link = NULL;
                }
                if (cmd->pre_cmd[i]->common) {
                    free(cmd->pre_cmd[i]->common);
                    cmd->pre_cmd[i]->common = NULL;
                }
                free(cmd->pre_cmd[i]);
                cmd->pre_cmd[i] = NULL;
            }
        }
        free(cmd->pre_cmd);
        cmd->pre_cmd = NULL;
        cmd->number_of_pre = 0;
    }
    if (cmd->post_cmd && cmd->number_of_post > 0) {
        for (i=0; i<cmd->number_of_post; i++) {
            if (cmd->post_cmd[i]) {
                if (cmd->post_cmd[i]->link) {
                    free(cmd->post_cmd[i]->link);
                    cmd->post_cmd[i]->link = NULL;
                }
                if (cmd->post_cmd[i]->common) {
                    free(cmd->post_cmd[i]->common);
                    cmd->post_cmd[i]->common = NULL;
                }
                free(cmd->post_cmd[i]);
                cmd->post_cmd[i] = NULL;
            }
        }
        free(cmd->post_cmd);
        cmd->post_cmd = NULL;
        cmd->number_of_post = 0;
    }
    if (cmd->cell_cmd && cmd->number_of_cell > 0) {
        for (i=0; i<cmd->number_of_cell; i++) {
            if (cmd->cell_cmd[i]) {
                if (cmd->cell_cmd[i]->link) {
                    free(cmd->cell_cmd[i]->link);
                    cmd->cell_cmd[i]->link = NULL;
                }
                if (cmd->cell_cmd[i]->common) {
                    free(cmd->cell_cmd[i]->common);
                    cmd->cell_cmd[i]->common = NULL;
                }
                free(cmd->cell_cmd[i]);
                cmd->cell_cmd[i] = NULL;
            }
        }
        free(cmd->cell_cmd);
        cmd->cell_cmd = NULL;
        cmd->number_of_cell = 0;
    }
}

//菜单类型定义
typedef enum {
    CommonMenu = 0,
    TitleMenu,
    RootMenu,
    SubpicMenu,
    AudioMenu,
    AngleMenu,
    ChapterMenu,
} menu_type_t;

typedef enum {
    MenuPGC,
    TitlePGC,
    PgcitPGC,
} pgc_type_t;

typedef struct {
    uint32_t index;
    uint32_t entry_cell;
} program_t;

typedef struct {
    uint32_t number_of_programs;
    program_t *programs;
} program_map_t;

typedef struct {
    uint32_t index;
    uint32_t duration;

} cell_t;

typedef struct {
    uint32_t number_of_cells;
    cell_t *cells;
} cell_map_t;

//static void destroy_cell_map(cell_map_t *cell_map) {
//    if (cell_map == NULL) {
//        return ;
//    }

//    free(cell_map->cells);
//    cell_map->cells = NULL;
//    cell_map->number_of_cells = 0;
//}

//static void destroy_program_map(program_map_t *program_map) {
//    if (program_map == NULL) {
//        return ;
//    }
//    free(program_map->programs);
//    program_map->programs = NULL;
//    program_map->number_of_programs = 0;
//}

typedef struct {
    cmd_t *cmd;
} button_info_t;

typedef struct {
    uint32_t number_of_buttons;
    button_info_t *button_info;
} button_group_t;

#define MAX_BUTTON_GROUP 256
typedef struct {
    uint32_t number_of_button_group;
    button_group_t button_group[MAX_BUTTON_GROUP];
} pci_table_t;

static void destroy_pci_table(pci_table_t *pci_table) {
    uint32_t button_group_index;
    if (pci_table == NULL) {
        return;
    }

    for (button_group_index = 0; button_group_index < pci_table->number_of_button_group; button_group_index++) {
        uint32_t button_index;
        button_group_t *button_group = &pci_table->button_group[button_group_index];
        if (button_group->button_info) {
            for (button_index = 0; button_index < button_group->number_of_buttons; button_index++) {
                if (button_group->button_info[button_index].cmd) {
                    if (button_group->button_info[button_index].cmd->link) {
                        free(button_group->button_info[button_index].cmd->link);
                    }
                    free(button_group->button_info[button_index].cmd);
                }
            }
            free(button_group->button_info);
        }
    }
    free(pci_table);
}

typedef struct {
    pgc_type_t pgc_type;
    union {
        menu_type_t menu_type;
        uint32_t pgc_index;
    };
    uint32_t duration;
    cmd_table_t cmd;

    union {
        pci_table_t *pci_table;
        void *unknow;
    };

//    program_map_t programs_map;
//    cell_map_t cells_map;

    uint32_t jump_count;
    uint32_t jump_program_count;
    uint32_t jump_cell_count;

    uint32_t vts;
} simple_pgc_t;

typedef struct {
    simple_pgc_t fp;
    uint32_t number_of_vmgm;
    simple_pgc_t *vmgm;
} vmg_t;

typedef struct {
    uint32_t pgcn;
    uint32_t pgn;
} ptt_t;

typedef struct {
    uint32_t number_of_ptt;
    ptt_t *ptts;
} _ttu_t;

typedef struct {
    uint32_t number_of_vtsm;
    simple_pgc_t *vtsm;
    uint32_t number_of_vtst;
    simple_pgc_t *vtst;
    uint32_t number_of_ttu;
    _ttu_t *ttus;

} vts_t;

typedef struct {
    vmg_t vmg;
    uint32_t number_of_vts;
    vts_t *vts;
} dvd_pgc_t;

typedef struct {
    uint32_t pgcn;
    uint32_t pgn;
    uint32_t entry;
} _chapter_t;

typedef struct {
    uint32_t vts;
    uint32_t ttn;
    uint32_t number_of_chapter;
    _chapter_t *chapters;

    uint32_t jump_count;
} _title_t;

typedef struct {
    uint32_t number_of_titles;
    _title_t *titles;
} _titles_t;

//获取菜单类型
static menu_type_t get_menu_type(uint32_t entry_id) {
    switch (entry_id & 0xf) {
    case 2:
        return TitleMenu;
    case 3:
        return RootMenu;
    case 4:
        return SubpicMenu;
    case 5:
        return AudioMenu;
    case 6:
        return AngleMenu;
    case 7:
        return ChapterMenu;
    default:
        return CommonMenu;
    }
}

static const char* get_menu_string(menu_type_t type) {
    switch (type) {
    case TitleMenu:
        return "Title Menu";
    case RootMenu:
        return "Root Menu";
    case SubpicMenu:
        return "SubpicMenu";
    case AudioMenu:
        return "AudioMenu";
    case AngleMenu:
        return "AngleMenu";
    case ChapterMenu:
        return "ChapterMenu";
    default:
        return "DefaultMenu";
    }
}

//dvd指令操作
static uint64_t get_instruction(const uint8_t *bytes) {
    uint64_t instruction = 0;
    instruction = ( (uint64_t) bytes[0] << 56 ) |
        ( (uint64_t) bytes[1] << 48 ) |
        ( (uint64_t) bytes[2] << 40 ) |
        ( (uint64_t) bytes[3] << 32 ) |
        ( (uint64_t) bytes[4] << 24 ) |
        ( (uint64_t) bytes[5] << 16 ) |
        ( (uint64_t) bytes[6] <<  8 ) |
          (uint64_t) bytes[7] ;
    return instruction;
}

static uint32_t getbits(uint64_t instruction, int32_t start, int32_t count) {
    uint64_t result = 0;
    uint64_t bit_mask = 0;
    int32_t  bits;

    if (count == 0) return 0;

    if ( ((start - count) < -1) ||
       (count > 32) ||
       (start > 63) ||
       (count < 0) ||
       (start < 0) ) {
        return 0;
    }
    /* all ones, please */
    bit_mask = ~bit_mask;
    bit_mask >>= 63 - start;
    bits = start + 1 - count;
    result = (instruction & bit_mask) >> bits;
    return (uint32_t) result;
}

static void print_cmd(const vm_cmd_t* cmd) {
    return ;
    logger_log(logger, LOGGER_DEBUG, "[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]",
           cmd->bytes[0], cmd->bytes[1], cmd->bytes[2], cmd->bytes[3],
           cmd->bytes[4], cmd->bytes[5], cmd->bytes[6], cmd->bytes[7]);
}

static char *linkcmd2str(link_cmd_t cmd) {
    switch (cmd) {
    case LinkNoLink:
        return "LinkNoLink";
    case LinkTopC:
        return "LinkTopC";
    case LinkNextC:
        return "LinkNextC";
    case LinkPrevC:
        return "LinkPrevC";
    case LinkTopPG:
        return "LinkTopPG";
    case LinkNextPG:
        return "LinkNextPG";
    case LinkPrevPG:
        return "LinkPrevPG";
    case LinkTopPGC:
        return "LinkTopPGC";
    case LinkNextPGC:
        return "LinkNextPGC";
    case LinkPrevPGC:
        return "LinkPrevPGC";
    case LinkGoUpPGC:
        return "LinkGoUpPGC";
    case LinkTailPGC:
        return "LinkTailPGC";
    case LinkRSM:
        return "LinkRSM";
    case LinkPGCN:
        return "LinkPGCN";
    case LinkPTTN:
        return "LinkPTTN";
    case LinkPGN:
        return "LinkPGN";
    case LinkCN:
        return "LinkCN";
    case Exit:
        return "Exit";
    case JumpTT:
        return "JumpTT";
    case JumpVTS_TT:
        return "JumpVTS_TT";
    case JumpVTS_PTT:
        return "JumpVTS_PTT";
    case JumpSS_FP:
        return "JumpSS_FP";
    case JumpSS_VMGM_MENU:
        return "JumpSS_VMGM_MENU";
    case JumpSS_VTSM:
        return "JumpSS_VTSM";
    case JumpSS_VMGM_PGC:
        return "JumpSS_VMGM_PGC";
    case CallSS_FP:
        return "CallSS_FP";
    case CallSS_VMGM_MENU:
        return "CallSS_VMGM_MENU";
    case CallSS_VTSM:
        return "CallSS_VTSM";
    case CallSS_VMGM_PGC:
        return "CallSS_VMGM_PGC";
    case PlayThis:
        return "PlayThis";
    }
    return "*** (bug)";
}

void print_link(uint32_t line, link_t* value) {
    char *cmd = linkcmd2str(value->command);

    switch (value->command) {
    case LinkNoLink:
    case LinkTopC:
    case LinkNextC:
    case LinkPrevC:
    case LinkTopPG:
    case LinkNextPG:
    case LinkPrevPG:
    case LinkTopPGC:
    case LinkNextPGC:
    case LinkPrevPGC:
    case LinkGoUpPGC:
    case LinkTailPGC:
    case LinkRSM:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s (button %d)", line, cmd, value->data1);
        break;
    case LinkPGCN:
    case JumpTT:
    case JumpVTS_TT:
    case JumpSS_VMGM_MENU: /*  == 2 -> Title Menu */
    case JumpSS_VMGM_PGC:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s %d", line, cmd, value->data1);
        break;
    case LinkPTTN:
    case LinkPGN:
    case LinkCN:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s %d (button %d)", line, cmd, value->data1, value->data2);
        break;
    case Exit:
    case JumpSS_FP:
    case PlayThis: /*  Humm.. should we have this at all.. */
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s", line, cmd);
        break;
    case JumpVTS_PTT:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s %d:%d", line, cmd, value->data1, value->data2);
        break;
    case JumpSS_VTSM:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s vts %d title %d menu %d",
            line, cmd, value->data1, value->data2, value->data3);
        break;
    case CallSS_FP:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s resume cell %d", line, cmd, value->data1);
        break;
    case CallSS_VMGM_MENU: /*  == 2 -> Title Menu */
    case CallSS_VTSM:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s %d resume cell %d", line, cmd, value->data1, value->data2);
        break;
    case CallSS_VMGM_PGC:
        logger_log(logger, LOGGER_DEBUG, "dvd_cmd[%d]: %s %d resume cell %d", line, cmd, value->data1, value->data2);
        break;
    }
}

static cmd_t* jump_instruction(uint64_t command) {
    cmd_t *cmd = (cmd_t*)calloc(1, sizeof(cmd_t));
    if (cmd == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    cmd->type = Link_Command;
    cmd->link = (link_t*)calloc(1, sizeof(link_t));
    if (cmd->link == NULL) {
        free(cmd);
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        return NULL;
    }

    switch(getbits(command, 51, 4)) {
    case 1:
        cmd->link->command = Exit;
        break;
    case 2:
        cmd->link->command = JumpTT;
        cmd->link->data1 = getbits(command, 22, 7);
        break;
    case 3:
        cmd->link->command = JumpVTS_TT;
        cmd->link->data1 = getbits(command, 22, 7);
        break;
    case 5:
        cmd->link->command = JumpVTS_PTT;
        cmd->link->data1 = getbits(command, 22, 7);
        cmd->link->data2 = getbits(command, 41, 10);
        break;
    case 6:
        switch(getbits(command, 23, 2)) {
        case 0:
            cmd->link->command = JumpSS_FP;
            break;
        case 1:
            cmd->link->command = JumpSS_VMGM_MENU;
            cmd->link->data1 =  getbits(command, 19, 4);
            break;
        case 2:
            cmd->link->command = JumpSS_VTSM;
            cmd->link->data1 =  getbits(command, 31, 8);
            cmd->link->data2 =  getbits(command, 39, 8);
            cmd->link->data3 =  getbits(command, 19, 4);
            break;
        case 3:
            cmd->link->command = JumpSS_VMGM_PGC;
            cmd->link->data1 =  getbits(command, 46, 15);
            break;
        }
        break;
    case 8:
        switch(getbits(command, 23, 2)) {
        case 0:
            cmd->link->command = CallSS_FP;
            cmd->link->data1 = getbits(command, 31, 8);
            break;
        case 1:
            cmd->link->command = CallSS_VMGM_MENU;
            cmd->link->data1 = getbits(command, 19, 4);
            cmd->link->data2 = getbits(command, 31, 8);
            break;
        case 2:
            cmd->link->command = CallSS_VTSM;
            cmd->link->data1 = getbits(command, 19, 4);
            cmd->link->data2 = getbits(command, 31, 8);
            break;
        case 3:
            cmd->link->command = CallSS_VMGM_PGC;
            cmd->link->data1 = getbits(command, 46, 15);
            cmd->link->data2 = getbits(command, 31, 8);
            break;
        }
        break;
    }

    print_link(0, cmd->link);
    return cmd;
}

static cmd_t* link_subins(uint64_t command, cmd_t *cmd) {
    uint16_t button = getbits(command, 15, 6);
    uint8_t  linkop = getbits(command, 4, 5);

    if(linkop > 0x10) {
        if (cmd) {
            if (cmd->link) {
                free(cmd->link);
                cmd->link = NULL;
            }
            free(cmd);
            cmd = NULL;
        }
        logger_log(logger, LOGGER_WARNING, " Unknown Link by Sub-Instruction command");
        return NULL;    /*  Unknown Link by Sub-Instruction command */
    }



   /*  Assumes that the link_cmd_t enum has the same values as the LinkSIns codes */
    cmd->link->command = linkop;
    cmd->link->data1 = button;

    print_link(0, cmd->link);
    return cmd;
}

static cmd_t* link_instruction(uint64_t command) {
    cmd_t *cmd = (cmd_t*)calloc(1, sizeof(cmd_t));
    if (cmd == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    cmd->type = Link_Command;
    cmd->link = (link_t*)calloc(1, sizeof(link_t));
    if (cmd->link == NULL) {
        free(cmd);
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        return NULL;
    }

    switch (getbits(command, 51, 4)) {
    case 1:
        return link_subins(command, cmd);
    case 4:
        cmd->link->command = LinkPGCN;
        cmd->link->data1 = getbits(command, 14, 15);
        break;
    case 5:
        cmd->link->command = LinkPTTN;
        cmd->link->data1 = getbits(command, 9, 10);
        cmd->link->data2 = getbits(command, 15, 6);
        break;
    case 6:
        cmd->link->command = LinkPGN;
        cmd->link->data1 = getbits(command, 6, 7);
        cmd->link->data2 = getbits(command, 15, 6);
        break;
    case 7:
        cmd->link->command = LinkCN;
        cmd->link->data1 = getbits(command, 7, 8);
        cmd->link->data2 = getbits(command, 15, 6);
        break;
    }

    print_link(0, cmd->link);
    return cmd;
}

typedef enum {
    Special_Instructions,       /*  Special instructions */
    Link_Jump_Instructions,     /*  Link/jump instructions */
    System_Set_Instructions,    /*  System set instructions */
    Set_Instructions,           /*  Set instructions, either Compare or Link may be used */
    Set_Compare_Instructions,   /*  Set, Compare -> Link Sub-Instruction */
    Compare_Instructions,       /*  Compare -> (Set and Link Sub-Instruction) */
    Compare_Set_Instructions,   /*  Compare -> Set, allways Link Sub-Instruction */
} pgc_instructions_t;

static cmd_t* tansform_cmd(const vm_cmd_t* cmd) {
    uint64_t instruction = get_instruction(cmd->bytes);

    switch(getbits(instruction, 63, 3)) {
    case Special_Instructions:
        print_cmd(cmd);
        break;
    case Link_Jump_Instructions:
        if (getbits(instruction, 60, 1)) {
            return jump_instruction(instruction);
        }
        else {
            return link_instruction(instruction);
        }
        break;
    case System_Set_Instructions:
        print_cmd(cmd);
        break;
    case Set_Instructions:
        print_cmd(cmd);
        break;
    case Set_Compare_Instructions:
        print_cmd(cmd);
        break;
    case Compare_Instructions:
        print_cmd(cmd);
        break;
    case Compare_Set_Instructions:
        print_cmd(cmd);
        break;
    default:
        print_cmd(cmd);
        break;
    }
    return NULL;
}



//static bool init_cell_map(uint8_t number_of_cell, cell_playback_t *cell_playback, cell_map_t *cell_map) {
//    uint8_t i;

//    if (number_of_cell == 0 || cell_playback == NULL || cell_map == NULL) {
//        //logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
//        return false;
//    }

//    cell_map->number_of_cells = number_of_cell;
//    cell_map->cells = (cell_t*)calloc(number_of_cell, sizeof(cell_t));
//    if (cell_map->cells == NULL) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
//        return false;
//    }

//    for (i=0; i<number_of_cell; i++) {
//        cell_map->cells[i].index = i+1;
//        cell_map->cells[i].duration = playbacktimetosec(&cell_playback[i].playback_time);
//    }

//    return true;
//}

//static bool init_program_map(uint8_t number_of_program, pgc_program_map_t  *pgc_program_map, program_map_t *program_map) {
//    uint8_t i;

//    if (number_of_program == 0 || pgc_program_map == NULL || program_map == NULL) {
//        //logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
//        return false;
//    }
//    program_map->number_of_programs = number_of_program;
//    program_map->programs = (program_t*)calloc(number_of_program, sizeof(program_t));
//    if (program_map->programs == NULL) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
//        return false;
//    }

//    for (i=0; i<number_of_program; i++) {
//        program_map->programs[i].index = i+1;
//        program_map->programs[i].entry_cell = pgc_program_map[i];
//    }
//    return true;
//}

//static bool init_vm_cmd_table(pci_t *pci, pci_table_t *pci_table) {
//    uint32_t button_index;
//    button_group_t *button_group = NULL;
//    if (pci == NULL || pci_table == NULL) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
//        return false;
//    }

//    if (pci_table->number_of_button_group >= MAX_BUTTON_GROUP) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): number of button group %d overflow",
//                   __FILE__, __LINE__, pci_table->number_of_button_group);
//        return  false;
//    }

//    button_group = &pci_table->button_group[pci_table->number_of_button_group];
//    button_group->number_of_buttons = pci->hli.hl_gi.btn_ns;
//    button_group->button_info = (button_info_t*)calloc(button_group->number_of_buttons, sizeof(button_info_t));
//    if (button_group->button_info == NULL) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
//        return false;

//    }
//    pci_table->number_of_button_group++;

//    logger_log(logger, LOGGER_DEBUG, "button command");
//    for (button_index = 0; button_index < button_group->number_of_buttons; button_index++) {
//        uint64_t instruction = get_instruction(pci->hli.btnit[button_index].cmd.bytes);
//        button_group->button_info[button_index].cmd = jump_instruction(instruction);
//    }
//    return true;
//}

static bool init_cmd_table(pgc_command_tbl_t *command_tbl, cmd_table_t *cmd_table) {
    if (command_tbl == NULL || cmd_table == NULL) {
        //logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return false;
    }

    if (command_tbl->nr_of_pre > 0 && command_tbl->pre_cmds) {
        cmd_table->number_of_pre = command_tbl->nr_of_pre;
        cmd_table->pre_cmd = (cmd_t**)calloc(cmd_table->number_of_pre, sizeof(cmd_t*));
        if (cmd_table->pre_cmd == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
            return false;
        }
        logger_log(logger, LOGGER_DEBUG, "pre command");
        for (uint32_t i=0; i<cmd_table->number_of_pre; i++) {
            cmd_table->pre_cmd[i] = tansform_cmd(&command_tbl->pre_cmds[i]);
        }
    }

    if (command_tbl->nr_of_post > 0 && command_tbl->post_cmds) {
        cmd_table->number_of_post = command_tbl->nr_of_post;
        cmd_table->post_cmd = (cmd_t**)calloc(cmd_table->number_of_post, sizeof(cmd_t**));
        if (cmd_table->post_cmd == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
            return false;
        }
        logger_log(logger, LOGGER_DEBUG, "post command");
        for (uint32_t i=0; i<cmd_table->number_of_post; i++) {
            cmd_table->post_cmd[i] = tansform_cmd(&command_tbl->post_cmds[i]);
        }
    }

    if (command_tbl->nr_of_cell > 0 && command_tbl->cell_cmds) {
        cmd_table->number_of_cell = command_tbl->nr_of_cell;
        cmd_table->cell_cmd = (cmd_t**)calloc(cmd_table->number_of_cell, sizeof(cmd_t*));
        if (cmd_table->cell_cmd == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
            return false;
        }
        logger_log(logger, LOGGER_DEBUG, "cell command");
        for (uint32_t i=0; i<cmd_table->number_of_cell; i++) {
            cmd_table->cell_cmd[i] = tansform_cmd(&command_tbl->cell_cmds[i]);
        }
    }
    return true;
}

static void destroy_dvd_pgc(dvd_pgc_t *dvd_pgc) {
    uint32_t i, j;
    if (dvd_pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }

    destroy_cmd_table(&dvd_pgc->vmg.fp.cmd);
    if (dvd_pgc->vmg.vmgm) {
        for (i=0; i<dvd_pgc->vmg.number_of_vmgm; i++) {
            destroy_cmd_table(&dvd_pgc->vmg.vmgm[i].cmd);
            destroy_pci_table(dvd_pgc->vmg.vmgm[i].pci_table);
            dvd_pgc->vmg.vmgm[i].pci_table = NULL;
        }
        free(dvd_pgc->vmg.vmgm);
    }

    if (dvd_pgc->vts) {
        for (i=0; i<dvd_pgc->number_of_vts; i++) {
            if (dvd_pgc->vts[i].vtsm) {
                for (j=0; j<dvd_pgc->vts[i].number_of_vtsm; j++) {
                    destroy_cmd_table(&dvd_pgc->vts[i].vtsm[j].cmd);
                    destroy_pci_table(dvd_pgc->vts[i].vtsm[j].pci_table);
                    dvd_pgc->vts[i].vtsm[j].pci_table = NULL;
//                    destroy_cell_map(&dvd_pgc->vts[i].vtsm[j].cells_map);
//                    destroy_program_map(&dvd_pgc->vts[i].vtsm[j].programs_map);
                }
                free(dvd_pgc->vts[i].vtsm);
            }
            if (dvd_pgc->vts[i].vtst) {
                for (j=0; j<dvd_pgc->vts[i].number_of_vtst; j++) {
                    destroy_cmd_table(&dvd_pgc->vts[i].vtst[j].cmd);
//                    destroy_cell_map(&dvd_pgc->vts[i].vtst[j].cells_map);
//                    destroy_program_map(&dvd_pgc->vts[i].vtst[j].programs_map);
                }
                free(dvd_pgc->vts[i].vtst);
            }
            if (dvd_pgc->vts[i].ttus) {
                if (dvd_pgc->vts[i].ttus->ptts) {
                    free(dvd_pgc->vts[i].ttus->ptts);
                    dvd_pgc->vts[i].ttus->ptts = NULL;
                }
                free(dvd_pgc->vts[i].ttus);
                dvd_pgc->vts[i].ttus = NULL;
            }
        }
        free(dvd_pgc->vts);
    }
    free(dvd_pgc);
}
// 目前决定不分析vob文件，这段代码暂时屏蔽。
//static pci_table_t * init_pci_table(dvd_file_t *vob_file) {
//    pci_t pci;
//    pci_table_t *pci_table = NULL;
//    unsigned char buf[0x800];
//    ssize_t ret = 0;
//    ssize_t filesize = 0;
//    ssize_t offset = 0;

//    if (vob_file == NULL) {
//        return NULL;
//    }

//    filesize = DVDFileSize(vob_file);
//    if (filesize > 2000) {
//        logger_log(logger, LOGGER_WARNING, "menu vob siez %d", filesize);
//        filesize = 2000;
//    }

//    for (offset=0; offset<filesize; offset++) {
//        int32_t        bMpeg1 = 0;
//        uint32_t       nHeaderLen;
//        uint32_t       nPacketLen;
//        uint32_t       nStreamID;
//        unsigned char *p = buf;

//        memset(buf, 0, 0x800);
//        ret = DVDReadBlocks(vob_file, offset, 1, buf);
//        if (ret <= 0) {
//            break;
//        }
//        offset++;


//        /* we should now have a PES packet here */
//        if (p[0] || p[1] || (p[2] != 1)) {
//            logger_log(logger, LOGGER_DEBUG, "demux error! %02x %02x %02x (should be 0x000001)"
//                       ,p[0],p[1],p[2]);
//            continue;
//        }

//        if (p[3] == 0xBA) { /* program stream pack header */
//            int32_t nStuffingBytes;

//            bMpeg1 = (p[4] & 0x40) == 0;
//            if (bMpeg1) {
//                p += 12;
//            } else { /* mpeg2 */
//                nStuffingBytes = p[0xD] & 0x07;
//                p += 14 + nStuffingBytes;
//            }
//        }

//        if (p[3] == 0xbb) { /* program stream system header */
//            nHeaderLen = (p[4] << 8) | p[5];
//            p += 6 + nHeaderLen;
//        }

//        nPacketLen = p[4] << 8 | p[5];
//        nStreamID  = p[3];

//        nHeaderLen = 6;
//        p += nHeaderLen;

//        if (nStreamID == 0xbf) { /* Private stream 2 */
//            if(p[0] == 0x00) {
//                memset(&pci, 0, sizeof(pci_t));
//                navRead_PCI(&pci, p+1);

//                if (pci.hli.hl_gi.btn_ns > 0) {
//                    if (pci_table == NULL) {
//                        pci_table = (pci_table_t*)calloc(1, sizeof(pci_table_t));
//                    }

//                    init_vm_cmd_table(&pci, pci_table);
//                }
//            }

//            p += nPacketLen;

//            /* We should now have a DSI packet. */
//            if(p[6] == 0x01) {
//                nPacketLen = p[4] << 8 | p[5];
//                p += 6;
////                navRead_DSI(nav_dsi, p+1);
//            }
//            continue;
//        }
//    }

//    return pci_table;
//}

static dvd_pgc_t* init_dvd_pgc(ifo_data_t *ifo_data) {
    dvd_pgc_t *dvd_pgc = NULL;
    ifo_handle_t *video_ts = NULL;
    ifo_handle_t *vts = NULL;
    uint32_t number_of_vts = 0;
    uint32_t i = 0;

    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    number_of_vts = get_vts_number(ifo_data);
    if (number_of_vts == 0) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts array is empty", __FILE__, __LINE__);
        return NULL;
    }

    video_ts = get_ifo_data(ifo_data, 0);
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): cannot get video_ts handle", __FILE__, __LINE__);
        return NULL;
    }
    if (video_ts->first_play_pgc == NULL
            || video_ts->pgci_ut == NULL
            || video_ts->pgci_ut->nr_of_lus == 0
            || video_ts->pgci_ut->lu == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    if (video_ts->pgci_ut->lu[0].pgcit == NULL
            || video_ts->pgci_ut->lu[0].pgcit->nr_of_pgci_srp == 0
            || video_ts->pgci_ut->lu[0].pgcit->pgci_srp == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    dvd_pgc = (dvd_pgc_t*)calloc(1, sizeof(dvd_pgc_t));
    if (dvd_pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        return NULL;
    }

    //初始化VMGM
    dvd_pgc->vmg.fp.pgc_type = MenuPGC;
    dvd_pgc->vmg.fp.vts = 0;
    dvd_pgc->vmg.fp.pgc_type = 0;
    logger_log(logger, LOGGER_DEBUG, "\n=== First Play PGC");
    init_cmd_table(video_ts->first_play_pgc->command_tbl, &dvd_pgc->vmg.fp.cmd);

    dvd_pgc->vmg.number_of_vmgm = video_ts->pgci_ut->lu[0].pgcit->nr_of_pgci_srp;
    dvd_pgc->vmg.vmgm = (simple_pgc_t*)calloc(dvd_pgc->vmg.number_of_vmgm, sizeof(simple_pgc_t));
    if (dvd_pgc->vmg.vmgm == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        goto error;
    }

    for (i=0; i<dvd_pgc->vmg.number_of_vmgm; i++) {
        if (video_ts->pgci_ut->lu[0].pgcit->pgci_srp[i].pgc == NULL) {
            continue;
        }
        dvd_pgc->vmg.vmgm[i].pgc_type = MenuPGC;
        dvd_pgc->vmg.vmgm[i].menu_type = get_menu_type(video_ts->pgci_ut->lu[0].pgcit->pgci_srp[i].entry_id);
        if (dvd_pgc->vmg.vmgm[i].menu_type == TitleMenu) {
            //dvd_pgc->vmg.vmgm[i].pci_table = init_pci_table(get_vob_handle(ifo_data, 0));
        }
        dvd_pgc->vmg.vmgm[i].duration = playbacktimetosec(&video_ts->pgci_ut->lu[0].pgcit->pgci_srp[i].pgc->playback_time);
        dvd_pgc->vmg.vmgm[i].vts = 0;
        dvd_pgc->vmg.vmgm[i].pgc_index = i+1;

        logger_log(logger, LOGGER_DEBUG, "\n=== VMGM %02d PGC %s %s", i+1,
                   sec_to_timestring(dvd_pgc->vmg.vmgm[i].duration),
                   get_menu_string(dvd_pgc->vmg.vmgm[i].menu_type));
        init_cmd_table(video_ts->pgci_ut->lu[0].pgcit->pgci_srp[i].pgc->command_tbl,
                &dvd_pgc->vmg.vmgm[i].cmd);
    }

    //初始化VTS
    dvd_pgc->number_of_vts = number_of_vts;
    dvd_pgc->vts = (vts_t*)calloc(number_of_vts, sizeof(vts_t));
    if (dvd_pgc->vts == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        goto error;
    }

    for (i=0; i<number_of_vts; i++) {
        uint32_t j;

        vts = get_ifo_data(ifo_data, i+1);
        if (vts == NULL) {
            continue;
        }
        // 初始化VTSM
        if (vts->pgci_ut != NULL
                && vts->pgci_ut->lu != NULL
                && vts->pgci_ut->nr_of_lus > 0
                && vts->pgci_ut->lu[0].pgcit != NULL
                && vts->pgci_ut->lu[0].pgcit->nr_of_pgci_srp > 0
                && vts->pgci_ut->lu[0].pgcit->pgci_srp != NULL) {
            dvd_pgc->vts[i].number_of_vtsm = vts->pgci_ut->lu[0].pgcit->nr_of_pgci_srp;
            dvd_pgc->vts[i].vtsm = (simple_pgc_t*)calloc(dvd_pgc->vts[i].number_of_vtsm, sizeof(simple_pgc_t));
            if (dvd_pgc->vts[i].vtsm == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
                goto error;
            }

            for (j=0; j<vts->pgci_ut->lu[0].pgcit->nr_of_pgci_srp; j++) {
                if (vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc == NULL) {
                    continue;
                }
                dvd_pgc->vts[i].vtsm[j].pgc_type = MenuPGC;
                dvd_pgc->vts[i].vtsm[j].menu_type = get_menu_type(vts->pgci_ut->lu[0].pgcit->pgci_srp[j].entry_id);
                if (dvd_pgc->vts[i].vtsm[j].menu_type == RootMenu) {
                    //dvd_pgc->vts[i].vtsm[j].pci_table = init_pci_table(get_vob_handle(ifo_data, j+1));
                }
                dvd_pgc->vts[i].vtsm[j].duration = playbacktimetosec(&vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->playback_time);
                dvd_pgc->vts[i].vtsm[j].vts = i+1;
                dvd_pgc->vts[i].vtsm[j].pgc_index = j+1;

                logger_log(logger, LOGGER_DEBUG, "\n=== VTS %02d VTSM %02d PGC %s %s", i+1, j+1,
                           sec_to_timestring(dvd_pgc->vts[i].vtsm->duration),
                           get_menu_string(dvd_pgc->vts[i].vtsm->menu_type));
                init_cmd_table(vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->command_tbl,
                        &dvd_pgc->vts[i].vtsm[j].cmd);
//                init_cell_map(vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->nr_of_cells, vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->cell_playback,
//                              &dvd_pgc->vts[i].vtsm[j].cells_map);
//                init_program_map(vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->nr_of_programs, vts->pgci_ut->lu[0].pgcit->pgci_srp[j].pgc->program_map,
//                                 &dvd_pgc->vts[i].vtsm[j].programs_map);
            }
        }

        // 初始化VTST
        if (vts->vts_pgcit != NULL
                && vts->vts_pgcit->nr_of_pgci_srp > 0
                && vts->vts_pgcit->pgci_srp != NULL) {
            dvd_pgc->vts[i].number_of_vtst = vts->vts_pgcit->nr_of_pgci_srp;
            dvd_pgc->vts[i].vtst = (simple_pgc_t*)calloc(dvd_pgc->vts[i].number_of_vtst, sizeof(simple_pgc_t));
            if (dvd_pgc->vts[i].vtst == NULL) {
                logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
                goto error;
            }

            for (j=0; j<vts->vts_pgcit->nr_of_pgci_srp; j++) {
                if (vts->vts_pgcit->pgci_srp[j].pgc == NULL) {
                    continue;
                }

                dvd_pgc->vts[i].vtst[j].pgc_type = TitlePGC;
                //dvd_pgc->vts[i].vtst[j].title = get_title_by_vts(ifo_data, i+1, vts->vts_pgcit->pgci_srp[j].entry_id & 0x0f);
                dvd_pgc->vts[i].vtst[j].duration = playbacktimetosec(&vts->vts_pgcit->pgci_srp[j].pgc->playback_time);
                dvd_pgc->vts[i].vtst[j].vts = i+1;
                dvd_pgc->vts[i].vtst[j].pgc_index = j+1;

                logger_log(logger, LOGGER_DEBUG, "\n=== VTS %02d VTST %02d  %s", i+1, j+1, sec_to_timestring(dvd_pgc->vts[i].vtst[j].duration));
                init_cmd_table(vts->vts_pgcit->pgci_srp[j].pgc->command_tbl, &dvd_pgc->vts[i].vtst[j].cmd);
//                init_cell_map(vts->vts_pgcit->pgci_srp[j].pgc->nr_of_cells, vts->vts_pgcit->pgci_srp[j].pgc->cell_playback,
//                              &dvd_pgc->vts[i].vtst[j].cells_map);
//                init_program_map(vts->vts_pgcit->pgci_srp[j].pgc->nr_of_programs, vts->vts_pgcit->pgci_srp[j].pgc->program_map,
//                                 &dvd_pgc->vts[i].vtst[j].programs_map);
            }

            dvd_pgc->vts[i].number_of_ttu = vts->vts_ptt_srpt->nr_of_srpts;
            dvd_pgc->vts[i].ttus = (_ttu_t*)calloc(vts->vts_ptt_srpt->nr_of_srpts, sizeof(_ttu_t));
            if (dvd_pgc->vts[i].ttus == NULL) {
                logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
                goto error;
            }

            for (j=0; j<vts->vts_ptt_srpt->nr_of_srpts; j++) {
                ttu_t *title = &vts->vts_ptt_srpt->title[j];
                dvd_pgc->vts[i].ttus[j].number_of_ptt = title->nr_of_ptts;
                dvd_pgc->vts[i].ttus[j].ptts = (ptt_t*)calloc(title->nr_of_ptts, sizeof(ptt_t));
                for (int n=0; n<title->nr_of_ptts; n++) {
                    dvd_pgc->vts[i].ttus[j].ptts[n].pgcn = title->ptt[n].pgcn;
                    dvd_pgc->vts[i].ttus[j].ptts[n].pgn = title->ptt[n].pgn;
                }
            }
        }
    }

    return dvd_pgc;

error:
    destroy_dvd_pgc(dvd_pgc);
    return NULL;
}

static void destroy_titles(_titles_t *titles) {
    if (titles == NULL) {
        return;
    }

    if (titles->titles) {
        for (uint32_t i=0; i<titles->number_of_titles; i++) {
            if (titles->titles[i].chapters) {
                free(titles->titles[i].chapters);
                titles->titles[i].chapters = NULL;
                titles->number_of_titles = 0;
            }
        }
        free(titles->titles);
        titles->titles = NULL;
        titles->number_of_titles = 0;
    }
}

static void print_titles(_titles_t *titles) {
    if (titles == NULL) {
        return;
    }

    if (titles->titles && titles->number_of_titles > 0) {
        for (uint32_t i=0; i<titles->number_of_titles; i++) {
            logger_log(logger, LOGGER_DEBUG, "Title %02d vts:%02d ttn:%02d -- %d", i+1,
                       titles->titles[i].vts, titles->titles[i].ttn, titles->titles[i].jump_count);
            if (titles->titles[i].chapters && titles->titles[i].number_of_chapter > 0) {
                for (uint32_t j=0; j<titles->titles[i].number_of_chapter; j++) {
                    logger_log(logger, LOGGER_DEBUG, "\tchapter %02d pgcn:%02d pg:%02d entry:%02d",
                               j+1, titles->titles[i].chapters[j].pgcn, titles->titles[i].chapters[j].pgn, titles->titles[i].chapters[j].entry);
                }
            }
        }
    }
}

static _titles_t *init_titles(ifo_data_t *ifo_data) {
    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    ifo_handle_t *video_ts = get_ifo_data(ifo_data, 0);
    if (video_ts == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): cannot get video_ts handle", __FILE__, __LINE__);
        return NULL;
    }

    if (video_ts->tt_srpt == NULL || video_ts->tt_srpt->nr_of_srpts == 0 || video_ts->tt_srpt->title == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): video_ts file error", __FILE__, __LINE__);
        return NULL;
    }

    _titles_t *titles = (_titles_t*)calloc(1, sizeof(_titles_t));
    if (titles == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        return NULL;
    }

    titles->number_of_titles = video_ts->tt_srpt->nr_of_srpts;
    titles->titles = (_title_t*)calloc(titles->number_of_titles, sizeof(_title_t));
    if (titles->titles == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        goto error;
    }

    for (uint32_t title_index = 0; title_index < video_ts->tt_srpt->nr_of_srpts; title_index++) {
        titles->titles[title_index].vts = video_ts->tt_srpt->title[title_index].title_set_nr;
        titles->titles[title_index].ttn = video_ts->tt_srpt->title[title_index].vts_ttn;
        titles->titles[title_index].number_of_chapter = video_ts->tt_srpt->title[title_index].nr_of_ptts;
        titles->titles[title_index].chapters = (_chapter_t*)calloc(titles->titles[title_index].number_of_chapter, sizeof(_chapter_t));


        ifo_handle_t *vts = get_ifo_data(ifo_data, titles->titles[title_index].vts);
        if (vts == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): cannt get vts %d handle", __FILE__, __LINE__, titles->titles[title_index].vts);
            continue;
        }

        if (vts->vts_ptt_srpt == NULL || titles->titles[title_index].ttn > vts->vts_ptt_srpt->nr_of_srpts) {
            logger_log(logger, LOGGER_ERR, "%s (%d): vts %d file error", __FILE__, __LINE__, ifo_data, titles->titles[title_index].vts);
            continue;
        }

        ttu_t *ttu = &vts->vts_ptt_srpt->title[titles->titles[title_index].ttn-1];

        if (titles->titles[title_index].number_of_chapter != ttu->nr_of_ptts) {
            logger_log(logger, LOGGER_ERR, "%s (%d): vts %d file error", __FILE__, __LINE__, ifo_data, titles->titles[title_index].vts);
            continue;
        }

        if (vts->vts_pgcit == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): vts %d file error", __FILE__, __LINE__, ifo_data, titles->titles[title_index].vts);
            continue;
        }

        for (uint32_t chapter_index = 0; chapter_index < titles->titles[title_index].number_of_chapter; chapter_index++) {
            titles->titles[title_index].chapters[chapter_index].pgcn = ttu->ptt[chapter_index].pgcn;
            titles->titles[title_index].chapters[chapter_index].pgn = ttu->ptt[chapter_index].pgn;

            if (ttu->ptt[chapter_index].pgcn > vts->vts_pgcit->nr_of_pgci_srp || vts->vts_pgcit->pgci_srp == NULL
                    || vts->vts_pgcit->pgci_srp[ttu->ptt[chapter_index].pgcn - 1].pgc == NULL
                    || ttu->ptt[chapter_index].pgn > vts->vts_pgcit->pgci_srp[ttu->ptt[chapter_index].pgcn - 1].pgc->nr_of_programs
                    || vts->vts_pgcit->pgci_srp[ttu->ptt[chapter_index].pgcn - 1].pgc->program_map == NULL) {
                logger_log(logger, LOGGER_ERR, "%s (%d): vts %d file error", __FILE__, __LINE__, titles->titles[title_index].vts);
                continue;
            }

            titles->titles[title_index].chapters[chapter_index].entry
                    = vts->vts_pgcit->pgci_srp[ttu->ptt[chapter_index].pgcn - 1].pgc->program_map[ttu->ptt[chapter_index].pgn - 1];
        }
    }


    return titles;

error:
    destroy_titles(titles);
    return NULL;
}

typedef struct {
    simple_pgc_t *pgc;
    uint32_t cur_vts;
} dvd_vm_t;

typedef struct {
    dvd_vm_t *vm;
    link_t *link;
    list_t *analyzer_stack;
    dvd_pgc_t *dvd_pgc;
    ifo_data_t *ifo_data;
    _titles_t *titles;
} track_parameter_t;

typedef bool (*track_pgc_cmd_fun)(track_parameter_t *track_parameter);

typedef struct {
    link_cmd_t link_cmd;
    track_pgc_cmd_fun track_fun;
} track_pgc_cmd_fun_table_t;

static bool push_pgc_to_stack(simple_pgc_t *pgc, uint32_t domain, list_t *analyzer_stack) {
    dvd_vm_t *vm = NULL;
    if (pgc == NULL || analyzer_stack == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return false;
    }

    //分析到的命令始终压栈
//    if (pgc->jump_count > 0) {
//        return false;
//    }

    vm = (dvd_vm_t*)calloc(1, sizeof(dvd_vm_t));
    if (vm == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): alloc error", __FILE__, __LINE__);
        return false;
    }

    vm->cur_vts = domain;
    vm->pgc = pgc;

    list_rpush(analyzer_stack, list_node_new(vm));
    return true;
}

//static bool check_track_parameter(track_parameter_t *track_parameter) {
//    if (track_parameter == NULL
//            || track_parameter->analyzer_stack == NULL
//            || track_parameter->dvd_pgc == NULL
//            || track_parameter->link == NULL
//            || track_parameter->ifo_data == NULL
//            || track_parameter->vm == NULL) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
//        return false;
//    }
//    return true;
//}

//TODO:添加跳转和链接命令分析函数

static bool track_jumpss_vmgm_pgc(track_parameter_t *track_parameter) {
    if (track_parameter->dvd_pgc->vmg.number_of_vmgm >= track_parameter->link->data1
            && track_parameter->dvd_pgc->vmg.vmgm != NULL
            && track_parameter->link->data1 > 0) {
        logger_log(logger, LOGGER_DEBUG, "push VMGM %d to stack", track_parameter->link->data1);
        push_pgc_to_stack(&track_parameter->dvd_pgc->vmg.vmgm[track_parameter->link->data1-1],
                0, track_parameter->analyzer_stack);
        return true;
    }

    logger_log(logger, LOGGER_ERR, "%s (%d): vmgm %d out of range",
               __FILE__, __LINE__, track_parameter->link->data1);
    return false;
}

static bool track_link_topc(track_parameter_t *track_parameter) {
    //track_parameter->vm->pgc->vts;
    //track_parameter->vm->pgc->pgc_index;
    track_parameter->vm->pgc->jump_cell_count++;
    return true;
}

static bool track_link_pgcn(track_parameter_t *track_parameter) {
    vts_t *vts = NULL;
    if (track_parameter->dvd_pgc->number_of_vts < track_parameter->vm->cur_vts) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range", __FILE__, __LINE__, track_parameter->link->data1);
        return false;
    }

    if (track_parameter->vm->cur_vts == 0) {
        if (track_parameter->dvd_pgc->vmg.number_of_vmgm < track_parameter->link->data1
            || track_parameter->link->data1 < 1) {
            logger_log(logger, LOGGER_ERR, "%s (%d): vmgm %d out of range", __FILE__, __LINE__, track_parameter->link->data1);
            return false;
        }

        logger_log(logger, LOGGER_DEBUG, "push vmgm %d to stack", track_parameter->link->data1);
        push_pgc_to_stack(&track_parameter->dvd_pgc->vmg.vmgm[track_parameter->link->data1-1], 0,
                track_parameter->analyzer_stack);
    }
    else {
        vts = &track_parameter->dvd_pgc->vts[track_parameter->vm->cur_vts-1];
        if (vts == NULL) {
            logger_log(logger, LOGGER_ERR, "%s (%d): cannot get vts %d handle", __FILE__, __LINE__, track_parameter->link->data1);
            return false;
        }

        if (track_parameter->vm->pgc->pgc_type == MenuPGC) {
            if (vts->number_of_vtsm < track_parameter->link->data1 || track_parameter->link->data1 < 1) {
                logger_log(logger, LOGGER_ERR, "%s (%d): vts %d vtsm %d out of range", __FILE__, __LINE__, track_parameter->vm->cur_vts, track_parameter->link->data1);
                return false;
            }
            logger_log(logger, LOGGER_DEBUG, "push VTS %d VTSM %d to stack", track_parameter->vm->cur_vts, track_parameter->link->data1);
            push_pgc_to_stack(&vts->vtsm[track_parameter->link->data1-1],
                    track_parameter->vm->cur_vts, track_parameter->analyzer_stack);
        }
        else {
            if (vts->number_of_vtst < track_parameter->link->data1 || track_parameter->link->data1 < 1) {
                logger_log(logger, LOGGER_ERR, "%s (%d): vts %d vtst %d out of range", __FILE__, __LINE__, track_parameter->vm->cur_vts, track_parameter->link->data1);
                return false;
            }

            logger_log(logger, LOGGER_DEBUG, "push VTS %d VTST %d to stack", track_parameter->vm->cur_vts, track_parameter->link->data1);


            push_pgc_to_stack(&vts->vtst[track_parameter->link->data1-1], track_parameter->vm->cur_vts, track_parameter->analyzer_stack);

            // 查找对应的Title
            if (track_parameter->titles) {
                for (uint32_t i = 0; i < track_parameter->titles->number_of_titles; i++) {
                    _title_t *title = &track_parameter->titles->titles[i];
                    if (title->vts == track_parameter->vm->cur_vts) {
                        if (title->chapters && title->number_of_chapter > 0) {
                            for (uint32_t j = 0; j < title->number_of_chapter; j++) {
                                _chapter_t *chapter = &title->chapters[j];
                                if (chapter->pgcn == track_parameter->link->data1) {
                                    title->jump_count++;
                                    return true;

                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

static bool track_link_pgn(track_parameter_t *track_parameter) {
//    if (track_parameter->vm->pgc->programs_map.number_of_programs < track_parameter->link->data1) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d program %d out of range",
//                   __FILE__, __LINE__, track_parameter->vm->cur_vts, track_parameter->link->data1);
//        return false;
//    }

    track_parameter->vm->pgc->jump_program_count++;
    return true;
}

static bool track_link_cn(track_parameter_t *track_parameter) {
//    if (track_parameter->vm->pgc->cells_map.number_of_cells < track_parameter->link->data1) {
//        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d cell %d out of range",
//                   __FILE__, __LINE__, track_parameter->vm->cur_vts, track_parameter->link->data1);
//        return false;
//    }

    track_parameter->vm->pgc->jump_cell_count++;
    return true;
}

static bool track_jumpss_vtsm(track_parameter_t *track_parameter) {
    //link->data1 VTS索引
    //link->data2 菜单索引
    //link->data3 菜单类型
    vts_t *vts = NULL;
    menu_type_t menu_type = CommonMenu;
    uint32_t i;

    if (track_parameter->dvd_pgc->number_of_vts < track_parameter->link->data1
            || track_parameter->link->data1 < 1) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range", __FILE__, __LINE__, track_parameter->link->data1);
        return false;
    }

    vts = &track_parameter->dvd_pgc->vts[track_parameter->link->data1-1];
    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): cannot get vts %d handle", __FILE__, __LINE__, track_parameter->link->data1);
        return false;
    }

    //只分析第一个菜单
    if (track_parameter->link->data2 > 1) {
        return true;
    }

    menu_type = get_menu_type(track_parameter->link->data3);

    for (i=0; i<vts->number_of_vtsm; i++) {
        if (vts->vtsm[i].menu_type == menu_type) {
            logger_log(logger, LOGGER_DEBUG, "push VTS %d VTSM %s to stack", track_parameter->link->data1, get_menu_string(vts->vtsm[i].menu_type));
            push_pgc_to_stack(&vts->vtsm[i], track_parameter->link->data1, track_parameter->analyzer_stack);
            return true;
        }
    }
    logger_log(logger, LOGGER_WARNING, "cannot find %s in vts %d", get_menu_string(menu_type), track_parameter->link->data1);

    return true;
}

static bool track_jumpvts_ptt(track_parameter_t *track_parameter) {
    //link->data1 当前vts内的title索引
    //link->data2 title的chapter索引
    vts_t *vts = NULL;
    simple_pgc_t *vtst = NULL;

    if (track_parameter->dvd_pgc->number_of_vts < track_parameter->vm->cur_vts) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range",
                   __FILE__, __LINE__, track_parameter->vm->cur_vts);
        return false;
    }

    vts = &track_parameter->dvd_pgc->vts[track_parameter->vm->cur_vts-1];
    if (vts == NULL) {
        return false;
    }

    if (vts->number_of_vtst < track_parameter->link->data1
            || track_parameter->link->data1 < 1) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d vtst %d out of range",
                   __FILE__, __LINE__, track_parameter->vm->cur_vts, track_parameter->link->data1);
        return false;
    }


    vtst = &vts->vtst[track_parameter->link->data1-1];
    if (vtst == NULL) {
        return false;
    }

    vtst->jump_program_count++;

    //logger_log(logger, LOGGER_DEBUG, "===> Play Title %02d", vtst->title);
    push_pgc_to_stack(vtst, track_parameter->vm->cur_vts, track_parameter->analyzer_stack);

    return true;
}

static bool track_callss_vtsm(track_parameter_t *track_parameter) {
    //link->data1 调用的菜单
    //link->data2 要恢复的cell
    vts_t *vts = NULL;
    menu_type_t menu_type = CommonMenu;
    uint32_t i;

    if (track_parameter->dvd_pgc->number_of_vts < track_parameter->vm->cur_vts) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range",
                   __FILE__, __LINE__, track_parameter->vm->cur_vts);
        return false;
    }

    vts = &track_parameter->dvd_pgc->vts[track_parameter->vm->cur_vts-1];
    if (vts == NULL) {
        return false;
    }

    menu_type = get_menu_type(track_parameter->link->data1);
    for (i=0; i<vts->number_of_vtsm; i++) {
        if (vts->vtsm[i].menu_type == menu_type) {
            logger_log(logger, LOGGER_DEBUG, "push VTS %d VTSM %s to stack",
                       track_parameter->vm->cur_vts, get_menu_string(vts->vtsm[i].menu_type));
            push_pgc_to_stack(&vts->vtsm[i], track_parameter->vm->cur_vts, track_parameter->analyzer_stack);
            return true;
        }
    }

    logger_log(logger, LOGGER_ERR, "cannot find %s in vts %d", get_menu_string(menu_type), track_parameter->vm->cur_vts);
    return false;
}

static bool track_callss_vmgm_pgc(track_parameter_t *track_parameter) {
    //link->data1 vmgm索引
    //link->data2 要恢复的cell
    simple_pgc_t *vmgm = NULL;
    if (track_parameter->dvd_pgc->vmg.number_of_vmgm < track_parameter->link->data1
            || track_parameter->link->data1 < 1) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vmgm %d out of range",
                   __FILE__, __LINE__, track_parameter->link->data1);
        return false;
    }

    vmgm = &track_parameter->dvd_pgc->vmg.vmgm[track_parameter->link->data1-1];
    if (vmgm == NULL) {
        return false;
    }

    logger_log(logger, LOGGER_DEBUG, "push VMGM %d to stack", track_parameter->link->data1);
    push_pgc_to_stack(vmgm, 0, track_parameter->analyzer_stack);
    return true;
}

static bool track_callss_vmgm_menu(track_parameter_t *track_parameter) {
    //link->data1 跳转到的菜单类型
    //link->data2 要恢复的cell
    menu_type_t menu_type = CommonMenu;
    uint32_t i;

    menu_type = get_menu_type(track_parameter->link->data1);
    for (i=0; i<track_parameter->dvd_pgc->vmg.number_of_vmgm; i++) {
        if (track_parameter->dvd_pgc->vmg.vmgm[i].menu_type == menu_type) {
            logger_log(logger, LOGGER_DEBUG, "push VMGM %s to stack", get_menu_string(menu_type));
            push_pgc_to_stack(&track_parameter->dvd_pgc->vmg.vmgm[i], 0, track_parameter->analyzer_stack);
            return true;
        }
    }
    return false;
}

static bool track_jumpvts_tt(track_parameter_t *track_parameter) {
    //link->data1 vts中的title索引
    vts_t *vts = NULL;
    if (track_parameter->dvd_pgc->number_of_vts < track_parameter->vm->cur_vts) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range", __FILE__, __LINE__, track_parameter->vm->cur_vts);
        return false;
    }

    vts = &track_parameter->dvd_pgc->vts[track_parameter->vm->cur_vts-1];
    if (vts == NULL) {
        return false;
    }

    for (uint32_t i = 0; i < track_parameter->titles->number_of_titles; i++) {
        _title_t *title = &track_parameter->titles->titles[i];
        if (title->vts == track_parameter->vm->cur_vts && title->ttn == track_parameter->link->data1) {
            title->jump_count++;
            if (title->chapters && title->number_of_chapter > 0) {
                if (title->chapters[0].pgcn > vts->number_of_vtst) {
                    return false;
                }

                push_pgc_to_stack(&vts->vtst[title->chapters[0].pgcn - 1], track_parameter->vm->cur_vts, track_parameter->analyzer_stack);
                return true;
            }
        }
    }

    return false;
}

static bool track_jumptt(track_parameter_t *track_parameter) {
    //link->data1 Title索引
    uint32_t vts_num, ttn_num;
    vts_t *vts = NULL;
    get_vts_ttn_by_title(track_parameter->ifo_data, track_parameter->link->data1, &vts_num, &ttn_num);
    logger_log(logger, LOGGER_DEBUG, "push VTS %d TTN %d to stack [Title:%d]", vts_num, ttn_num, track_parameter->link->data1);

    if (vts_num == 0 || ttn_num == 0) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return false;
    }

    if (track_parameter->dvd_pgc->number_of_vts < vts_num) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d out of range", __FILE__, __LINE__, vts_num);
        return false;
    }
    vts = &track_parameter->dvd_pgc->vts[vts_num-1];

    if (vts == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): cannot get vts %d handle", __FILE__, __LINE__, vts_num);
        return false;
    }

    if (vts->number_of_ttu < ttn_num) {
        logger_log(logger, LOGGER_ERR, "%s (%d): vts %d ttn %d out of range", __FILE__, __LINE__, vts_num, ttn_num);
        return false;
    }

    _title_t *title = &track_parameter->titles->titles[track_parameter->link->data1 - 1];
    title->jump_count++;
    if (title->chapters != NULL && title->number_of_chapter > 0) {
        if (title->chapters[0].pgcn <= vts->number_of_vtst) {
            push_pgc_to_stack(&vts->vtst[title->chapters[0].pgcn - 1], vts_num, track_parameter->analyzer_stack);
            return true;
        }
    }

    return false;
}

//static bool track_jumpss_vmgm_menu(track_parameter_t *track_parameter) {
//    uint32_t i;

//    menu_type_t menu_type = get_menu_type(track_parameter->link->data1);
//    for (i=0; i<track_parameter->dvd_pgc->vmg.number_of_vmgm; i++) {
//        simple_pgc_t *vmgm = &track_parameter->dvd_pgc->vmg.vmgm[i];
//        if (vmgm == NULL) {
//            continue;
//        }
//        if (menu_type == vmgm->menu_type) {
//            logger_log(logger, LOGGER_DEBUG, "push VMGM %s to stack", get_menu_string(menu_type));
//            push_pgc_to_stack(vmgm, 0, track_parameter->analyzer_stack);
//            return true;
//        }
//    }
//    return false;
//}

static bool track_link_unknow(track_parameter_t *track_parameter) {
    if (track_parameter == NULL){
        return false;
    }
    return true;
}

track_pgc_cmd_fun_table_t track_pgc_cmd_fun_table[] = {
    {JumpSS_VMGM_PGC, track_jumpss_vmgm_pgc},
    {JumpSS_VTSM, track_jumpss_vtsm},
    {JumpSS_FP, track_link_unknow},     //跳到first paly pgc 不用处理了。
    {JumpSS_VMGM_MENU, track_link_unknow},     //跳到first paly pgc 不用处理了。
//    {JumpVTS_PTT, track_jumpvts_ptt},
    {JumpVTS_TT, track_jumpvts_tt},
    {JumpTT, track_jumptt},
    {CallSS_VTSM, track_callss_vtsm},
    {CallSS_FP, track_link_unknow},
    {CallSS_VMGM_PGC, track_callss_vmgm_pgc},
//    {CallSS_VMGM_MENU, track_callss_vmgm_menu},
//    {LinkCN, track_link_cn}, //_Angels and Demons_.iso
//    {LinkPGN, track_link_pgn},
    {LinkPGCN, track_link_pgcn},
//    {LinkPGN, track_link_unknow}, //lg_combi_recorder.iso
    {LinkTailPGC, track_link_unknow}, // go to the post commands
    {LinkTopPG, track_link_unknow},
//    {LinkTopC, track_link_topc},
    {LinkRSM, track_link_unknow},
//    {LinkPrevPG, track_link_unknow},
    {Exit, track_link_unknow},
    {0, NULL},
};

static void analyzer_pgc_link_cmd(dvd_vm_t *vm, uint32_t cmd_index, link_t *link, list_t *analyzer_stack,
                                  dvd_pgc_t *dvd_pgc, ifo_data_t *ifo_data, _titles_t *titles) {
    uint32_t i=0;
    track_parameter_t track_parameter;

    if (vm == NULL || link == NULL || analyzer_stack == NULL
            || dvd_pgc == NULL || ifo_data == NULL || titles == NULL) {
        return;
    }

    track_parameter.analyzer_stack = analyzer_stack;
    track_parameter.dvd_pgc = dvd_pgc;
    track_parameter.link = link;
    track_parameter.ifo_data = ifo_data;
    track_parameter.vm = vm;
    track_parameter.titles = titles;

    if (vm->pgc->pgc_type == MenuPGC && vm->pgc->menu_type == TitleMenu) {
        logger_log(logger, LOGGER_DEBUG, "");
    }

    if (vm->pgc->pgc_type == TitlePGC) {
        //logger_log(logger, LOGGER_DEBUG, ">>>>>>> Analyzer VTST %d index %d cmd %d", vm->pgc->vts, vm->pgc->index, cmd_index);
    }
    else if (vm->pgc->pgc_type == MenuPGC) {
        //logger_log(logger, LOGGER_DEBUG, ">>>>>>> Analyzer VTSM %d index %d cmd %d", vm->pgc->vts, vm->pgc->index, cmd_index);
    }
    else {
        //logger_log(logger, LOGGER_DEBUG, ">>>>>>> Analyzer VTS %d index %d cmd %d", vm->pgc->vts, vm->pgc->index, cmd_index);
    }

    while(1) {
        if (track_pgc_cmd_fun_table[i].link_cmd == link->command
                &&  track_pgc_cmd_fun_table[i].track_fun != NULL) {
            print_link(cmd_index, link);
            if (track_pgc_cmd_fun_table[i].track_fun(&track_parameter)) {
                return;
            }
        }
        i++;

        if (track_pgc_cmd_fun_table[i].link_cmd == 0
                &&  track_pgc_cmd_fun_table[i].track_fun == NULL) {
            break;
        }
    }
}

static void analyzer_pgc_cmd(dvd_vm_t *vm, uint32_t number_of_cmd, cmd_t **cmd, list_t *analyzer_stack,
                             dvd_pgc_t *dvd_pgc, ifo_data_t *ifo_data, _titles_t *titles) {
    uint32_t i;
    if (vm == NULL || number_of_cmd == 0 || cmd == NULL || analyzer_stack == NULL
            || dvd_pgc == NULL || ifo_data == NULL || titles == NULL) {
        //logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }
    for (i=0; i<number_of_cmd; i++) {
        if (cmd[i] == NULL) {
            continue;
        }
        if (cmd[i]->type == Link_Command) {
            //print_link(i+1, cmd[i]->link);
            analyzer_pgc_link_cmd(vm, i+1, cmd[i]->link, analyzer_stack, dvd_pgc, ifo_data, titles);
        }
    }
}


static void analyzer_button_cmd(dvd_vm_t *vm, uint32_t number_of_button, button_info_t *button_info, list_t *analyzer_stack,
                             dvd_pgc_t *dvd_pgc, ifo_data_t *ifo_data, _titles_t *titles) {
    uint32_t i;
    if (vm == NULL || number_of_button == 0 || button_info == NULL
            || analyzer_stack == NULL || dvd_pgc == NULL || ifo_data == NULL) {
        //logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }

    for (i=0; i<number_of_button; i++) {
        if (button_info[i].cmd == NULL) {
            continue;
        }
        if (button_info[i].cmd->type == Link_Command) {
            //print_link(i+1, button_info[i].cmd->link);
            analyzer_pgc_link_cmd(vm, i+1, button_info[i].cmd->link, analyzer_stack, dvd_pgc, ifo_data, titles);
        }
    }
}


static void analyzer_pgc(dvd_vm_t *vm, list_t *analyzer_stack, dvd_pgc_t *dvd_pgc, ifo_data_t *ifo_data, _titles_t *titles) {
    if (vm == NULL || analyzer_stack == NULL || dvd_pgc == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }

    if (vm->pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }

    logger_log(logger, LOGGER_DEBUG, "pre command %d", vm->pgc->cmd.number_of_pre);
    analyzer_pgc_cmd(vm, vm->pgc->cmd.number_of_pre, vm->pgc->cmd.pre_cmd, analyzer_stack, dvd_pgc, ifo_data, titles);
    logger_log(logger, LOGGER_DEBUG, "post command %d", vm->pgc->cmd.number_of_post);
    analyzer_pgc_cmd(vm, vm->pgc->cmd.number_of_post, vm->pgc->cmd.post_cmd, analyzer_stack, dvd_pgc, ifo_data, titles);
    logger_log(logger, LOGGER_DEBUG, "cell command %d", vm->pgc->cmd.number_of_cell);
    analyzer_pgc_cmd(vm, vm->pgc->cmd.number_of_cell, vm->pgc->cmd.cell_cmd, analyzer_stack, dvd_pgc, ifo_data, titles);

    if (vm->pgc->pci_table) {
        uint32_t button_group_index;
        for (button_group_index = 0; button_group_index < vm->pgc->pci_table->number_of_button_group; button_group_index++) {
            button_group_t *button_group = &vm->pgc->pci_table->button_group[button_group_index];
            analyzer_button_cmd(vm, button_group->number_of_buttons, button_group->button_info, analyzer_stack, dvd_pgc, ifo_data, titles);
        }
    }

}

static void track_pgc(dvd_pgc_t *dvd_pgc, ifo_data_t *ifo_data, _titles_t *titles) {
    list_t *analyzer_stack = NULL;

    if (dvd_pgc == NULL || ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return;
    }

    analyzer_stack = list_new();
    analyzer_stack->free = free;

    logger_log(logger, LOGGER_DEBUG, "push First Player PGC to stack");
    push_pgc_to_stack(&dvd_pgc->vmg.fp, 0, analyzer_stack);

//    for (uint32_t i=0; i<dvd_pgc->vmg.number_of_vmgm; i++) {
//        if (dvd_pgc->vmg.vmgm[i].menu_type == TitleMenu) {
//            push_pgc_to_stack(&dvd_pgc->vmg.vmgm[i], 0, analyzer_stack);
//            break;
//        }
//    }


    while(analyzer_stack->len) {
        dvd_vm_t *vm = NULL;
        list_node_t *node = list_lpop(analyzer_stack);

        if (node == NULL) {
            continue;
        }

        vm = (dvd_vm_t*)node->val;
        if (vm == NULL) {
            continue;
        }

        if (vm->pgc->jump_count == 0) {
            analyzer_pgc(vm, analyzer_stack, dvd_pgc, ifo_data, titles);
        }
        else {
            //logger_log(logger, LOGGER_DEBUG, "<<<< VTS %d, index %d", vm->cur_vts, vm->pgc->index);
        }
        vm->pgc->jump_count++;

        free(vm);
    }


    list_destroy(analyzer_stack);
}

static void printf_pci_table(pci_table_t *pci_table) {
    if (pci_table) {
        for (uint32_t bgi=0; bgi<pci_table->number_of_button_group; bgi++) {
            button_group_t *button_group = &pci_table->button_group[bgi];
            for (uint32_t bi=0; bi<button_group->number_of_buttons; bi++) {
                print_link(bi, button_group->button_info[bi].cmd->link);
            }
        }
    }
}

struct pgc_analyzer_s{
    dvd_pgc_t *dvd_pgc;
    _titles_t *titles;
} ;

void destroy_pgc_analyzer(pgc_analyzer_t *analyzer) {
    if (analyzer == NULL) {
        return;
    }

    if (analyzer->dvd_pgc) {
        destroy_dvd_pgc(analyzer->dvd_pgc);

    }
    if (analyzer->titles) {
        destroy_titles(analyzer->titles);
    }
    free(analyzer);
}

pgc_analyzer_t* pgc_analyzer(ifo_data_t *ifo_data) {
    uint32_t i, j;

    if (ifo_data == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): parameter error", __FILE__, __LINE__);
        return NULL;
    }

    dvd_pgc_t *dvd_pgc = init_dvd_pgc(ifo_data);
    if (dvd_pgc == NULL) {
        logger_log(logger, LOGGER_ERR, "%s (%d): init dvd pgc error", __FILE__, __LINE__);
        return NULL;
    }

    _titles_t *titles = init_titles(ifo_data);
    if (titles == NULL) {
        destroy_dvd_pgc(dvd_pgc);
        return NULL;
    }

    track_pgc(dvd_pgc, ifo_data, titles);

    logger_log(logger, LOGGER_DEBUG,"TitleMenu");
    for (i=0; i<dvd_pgc->vmg.number_of_vmgm; i++) {
        printf_pci_table(dvd_pgc->vmg.vmgm[i].pci_table);
    }

    for (i=0; i<dvd_pgc->number_of_vts; i++) {
        for (j=0; j<dvd_pgc->vts[i].number_of_vtsm; j++) {
            logger_log(logger, LOGGER_DEBUG,"VTS %d VTSM %d", i+1, j+1);
            printf_pci_table(dvd_pgc->vts[i].vtsm[j].pci_table);
        }
    }

    logger_log(logger, LOGGER_DEBUG, "===========================");
    logger_log(logger, LOGGER_DEBUG, "===========================");
    logger_log(logger, LOGGER_DEBUG, "===========================");

    print_titles(titles);


    for (uint32_t i=0; i<titles->number_of_titles; i++) {
        logger_log(logger, LOGGER_DEBUG, "Title %02d jump: %d", i+1, titles->titles[i].jump_count);
    }

    pgc_analyzer_t *analyzer = (pgc_analyzer_t*)calloc(1, sizeof(pgc_analyzer_t));
    if (analyzer == NULL) {
        destroy_dvd_pgc(dvd_pgc);
        destroy_titles(titles);
    }

    analyzer->dvd_pgc = dvd_pgc;
    analyzer->titles = titles;


    return analyzer;
}

int *get_main_title_by_pgc(pgc_analyzer_t *analyzer, int *count) {
    if (analyzer == NULL || count == NULL)  {
        return NULL;
    }

    *count = 0;
    uint32_t max = 0;
    if (analyzer->titles && analyzer->titles->number_of_titles) {
        for (uint32_t i=0; i<analyzer->titles->number_of_titles; i++) {
            if (analyzer->titles->titles[i].jump_count > max) {
                max = analyzer->titles->titles[i].jump_count;
                *count = 1;
            }
            else if (analyzer->titles->titles[i].jump_count == max) {
                (*count)++;
            }
        }

        int *titles = (int*)calloc(*count, sizeof(int));
        if (titles == NULL) {
            return NULL;
        }

        *count = 0;
        for (uint32_t i=0; i<analyzer->titles->number_of_titles; i++) {
            if (analyzer->titles->titles[i].jump_count == max) {
                titles[(*count)++] = i + 1;
            }
        }

        return titles;
    }
    return NULL;
}












