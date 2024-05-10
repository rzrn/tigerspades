#define TOOLKIT "GLUT"

#define GLUT_SPECIAL_MASK        (1 << 30)
#define TOOLKIT_KEY_UP           ((int) 'W')
#define TOOLKIT_KEY_DOWN         ((int) 'S')
#define TOOLKIT_KEY_LEFT         ((int) 'A')
#define TOOLKIT_KEY_RIGHT        ((int) 'D')
#define TOOLKIT_KEY_JUMP         ((int) ' ')
#define TOOLKIT_KEY_SPRINT       ((int) 'F')
#define TOOLKIT_KEY_CURSOR_UP    GLUT_SPECIAL_MASK | GLUT_KEY_UP
#define TOOLKIT_KEY_CURSOR_DOWN  GLUT_SPECIAL_MASK | GLUT_KEY_DOWN
#define TOOLKIT_KEY_CURSOR_LEFT  GLUT_SPECIAL_MASK | GLUT_KEY_LEFT
#define TOOLKIT_KEY_CURSOR_RIGHT GLUT_SPECIAL_MASK | GLUT_KEY_RIGHT
#define TOOLKIT_KEY_BACKSPACE    ((int) '\b')
#define TOOLKIT_KEY_TOOL1        ((int) '1')
#define TOOLKIT_KEY_TOOL2        ((int) '2')
#define TOOLKIT_KEY_TOOL3        ((int) '3')
#define TOOLKIT_KEY_TOOL4        ((int) '4')
#define TOOLKIT_KEY_TAB          ((int) '\t')
#define TOOLKIT_KEY_ESCAPE       0x1B
#define TOOLKIT_KEY_MAP          ((int) 'M')
#define TOOLKIT_KEY_CROUCH       ((int) 'C')
#define TOOLKIT_KEY_SNEAK        ((int) 'V')
#define TOOLKIT_KEY_ENTER        ((int) '\r')
#define TOOLKIT_KEY_F1           GLUT_SPECIAL_MASK | GLUT_KEY_F1
#define TOOLKIT_KEY_F2           GLUT_SPECIAL_MASK | GLUT_KEY_F2
#define TOOLKIT_KEY_F3           GLUT_SPECIAL_MASK | GLUT_KEY_F3
#define TOOLKIT_KEY_F4           GLUT_SPECIAL_MASK | GLUT_KEY_F4
#define TOOLKIT_KEY_YES          ((int) 'Y')
#define TOOLKIT_KEY_NO           ((int) 'N')
#define TOOLKIT_KEY_VOLUME_UP    ((int) '+')
#define TOOLKIT_KEY_VOLUME_DOWN  ((int) '-')
#define TOOLKIT_KEY_RELOAD       ((int) 'R')
#define TOOLKIT_KEY_CHAT         ((int) 'T')
#define TOOLKIT_KEY_FULLSCREEN   GLUT_SPECIAL_MASK | GLUT_KEY_F11
#define TOOLKIT_KEY_SCREENSHOT   GLUT_SPECIAL_MASK | GLUT_KEY_F5
#define TOOLKIT_KEY_CHANGETEAM   ((int) ',')
#define TOOLKIT_KEY_CHANGEWEAPON ((int) '.')
#define TOOLKIT_KEY_PICKCOLOR    ((int) 'E')
#define TOOLKIT_KEY_COMMAND      ((int) '/')
#define TOOLKIT_KEY_HIDEHUD      GLUT_SPECIAL_MASK | GLUT_KEY_F6
#define TOOLKIT_KEY_LASTTOOL     ((int) 'Q')
#define TOOLKIT_KEY_NETWORKSTATS GLUT_SPECIAL_MASK | GLUT_KEY_F12
#define TOOLKIT_KEY_SAVE_MAP     GLUT_SPECIAL_MASK | GLUT_KEY_F9
#define TOOLKIT_KEY_SELECT1      ((int) '1')
#define TOOLKIT_KEY_SELECT2      ((int) '2')
#define TOOLKIT_KEY_SELECT3      ((int) '3')