#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/*======================================================================*/
/* BEGIN USER Define */

#define BYTE_BIT        6
#define SET_BIT         14
#define TAG_BIT         12
#define BYTE_MASK       0x0000003F
#define SET_MASK        0x000FFFC0

// Cache configuration
#define DATA_WAYS       4
#define INSTR_WAYS      2

// LRU values
#define DATA_LRU_MAX    (DATA_WAYS - 1)
#define DATA_LRU_MIN    0
#define INSTR_LRU_MAX   (INSTR_WAYS - 1)
#define INSTR_LRU_MIN   0

// Computed values
#define NUM_SETS        (1 << SET_BIT)
#define TAG_INVALID     ((1 << TAG_BIT) - 1)

// Trace file settings
#define MAX_TRACES      500

/* END USER Define */

/*======================================================================*/
/* BEGIN USER Typedef */

typedef enum {
    READ            = 0,
    WRITE           = 1,
    FETCH           = 2,
    EVICT           = 3,
    REQ_L2          = 4,
    RESET_AND_CLEAR = 8,
    PRINT_LOG       = 9
} Operation_Typedef;

typedef struct {
    uint16_t tag;
    uint16_t set;
    uint8_t LRU_State;
    uint8_t Valid;
    uint8_t Dirty;
    uint8_t First_Write_Done;
    uint32_t address;
} Cache_Line_Typedef;

typedef struct {
    uint32_t Data_Hit;
    uint32_t Data_Miss;
    uint32_t Write_Back;
    uint32_t Data_Read_Access;
    uint32_t Data_Write_Access;
    float Data_Hit_Ratio;
} Data_Cache_Stats_Typedef;

typedef struct {
    uint32_t Instruction_Hit;
    uint32_t Instruction_Miss;
    uint32_t Instruction_Read_Access;
    uint32_t Instruction_Write_Access;
    float Instr_Hit_Ratio;
} Instr_Cache_Stats_Typedef;

// Expected Results for Auto-Verification
typedef struct {
    char test_name[100];
    uint32_t expected_data_read;
    uint32_t expected_data_write;
    uint32_t expected_data_hit;
    uint32_t expected_data_miss;
    uint32_t expected_write_back;
    uint32_t expected_instr_read;
    uint32_t expected_instr_write;
    uint32_t expected_instr_hit;
    uint32_t expected_instr_miss;
    float expected_data_hit_ratio;
    float expected_instr_hit_ratio;
    int has_expected;  // Flag to check if metadata exists
} Expected_Results_Typedef;

/* END USER Typedef */

/*======================================================================*/
/* BEGIN USER Variable */

Cache_Line_Typedef Data_Cache[DATA_WAYS][NUM_SETS];
Cache_Line_Typedef Instr_Cache[INSTR_WAYS][NUM_SETS];

Data_Cache_Stats_Typedef  Data_Stats_Report;
Instr_Cache_Stats_Typedef Instr_Stats_Report;

unsigned int Mode = 3;
int Hit_Show = 0;

/* END USER Variable */

/*======================================================================*/
/* BEGIN USER PFP */

bool Reset_And_Clear_Cache();
unsigned int Selection_Menu();
FILE* Open_Trace_File(char* Trace_File);
bool Read_and_Run_Trace_File(FILE* fd);

//Cache operations
bool Data_Cache_Read(unsigned int address);
bool Data_Cache_Write(unsigned int address);
bool Instruction_Cache_Fetch(unsigned int address);
bool L2_Evict_Command_to_L1(unsigned int address);
bool Print_Content_And_State();

//Support functions
int Data_Match_Find(unsigned int Input_Tag, unsigned int Input_Set);
int Instruction_Match_Find(unsigned int Input_Tag, unsigned int Input_Set);
void Data_LRU_State_Update(unsigned int Set_Way, unsigned int Cache_Set);
void Instruction_LRU_State_Update(unsigned int Set_Way, unsigned int Cache_Set);
int Data_LRU_Smallest_Find(uint16_t Set_Index);
int Instruction_LRU_Smallest_Find(uint16_t Set_Index);

// Auto-verification functions
void Print_Expected_Results(Expected_Results_Typedef* expected);
bool Verify_Results(Expected_Results_Typedef* expected);

/* END USER PFP */

/*======================================================================*/
/* BEGIN MAIN PROGRAM */

int main(int argc, char* argv[])
{    
    char *trace_file_name;    
    FILE *fd = NULL;
    
    printf("\033[32m==============================================================================================================\033[0m\n");
    printf("\033[32m\t\t\t\t\033[4;1mINITIALIZATION:\033[0m\n");
    if (argc < 2)
    {
        printf("\033[31mERROR: Trace file name not found!\033[0m\n");
        exit(1);
    }
    else
    {
        trace_file_name = argv[1];
        printf("\033[32;4;1m1. Trace file name:\033[0m\033[32m %s\n\033[0m", trace_file_name);
    }
    if (argc > 2) Hit_Show = atoi(argv[2]);
    
    printf("\033[32;4;1m2. Resetting all cache lines and stats...\033[0m\n");
    if (Reset_And_Clear_Cache()) printf("\033[32m\t   => DONE\033[0m\n");
    else
    {
        printf("\033[31mERROR: Cannot reset and clear cache!\033[0m\n");
        exit(1);
    }

    Mode = (unsigned int) Selection_Menu();
    
    if ((fd = Open_Trace_File(trace_file_name))) 
        printf("\033[32;4;1m4. Trace file is opened successfully!\033[0m\n");
    else 
    {
        printf("\033[31mERROR: Cannot open trace file!\033[0m\n");
        exit(1);
    }
    
    printf("\033[32m==============================================================================================================\033[0m\n");
    
    if (Mode > 0) printf("\033[33m==============================================================================================================\033[0m\n");
    printf("\033[33m\t\t\t\t\033[4;1mMESSAGE BETWEEN L1 AND L2:\033[0m\n");
    if (Read_and_Run_Trace_File(fd) == false) 
        printf("\033[31mERROR: Cannot read and simulate trace file!\033[0m\n");
    
    fclose(fd);
    
    printf("\033[32;1m\t\t\t\t\t\tTEST FINISHED!\033[0m\n");

    return 0;
}
/* END MAIN PROGRAM */

/*======================================================================*/
/* BEGIN User function */

bool Reset_And_Clear_Cache()
{
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {                
        for(unsigned int j = 0; j < NUM_SETS; j++)
        {                    
            Data_Cache[i][j].tag = TAG_INVALID;
            Data_Cache[i][j].set = 0;
            Data_Cache[i][j].LRU_State = 0;
            Data_Cache[i][j].Valid = 0;
            Data_Cache[i][j].Dirty = 0;
            Data_Cache[i][j].First_Write_Done = 0;
            Data_Cache[i][j].address = 0;
        }                   
    }
    
    for (uint8_t i = 0; i < INSTR_WAYS; i++)
    {        
        for (unsigned int j = 0; j < NUM_SETS; j++)
        {
            Instr_Cache[i][j].tag = TAG_INVALID;
            Instr_Cache[i][j].set = 0;
            Instr_Cache[i][j].LRU_State = 0;
            Instr_Cache[i][j].Valid = 0;
            Instr_Cache[i][j].Dirty = 0;
            Instr_Cache[i][j].First_Write_Done = 0;
            Instr_Cache[i][j].address = 0;
        }        
    }
    
    Data_Stats_Report.Data_Hit = 0;
    Data_Stats_Report.Data_Miss = 0;
    Data_Stats_Report.Write_Back = 0;
    Data_Stats_Report.Data_Read_Access = 0;
    Data_Stats_Report.Data_Write_Access = 0;
    Data_Stats_Report.Data_Hit_Ratio = 0.0;
    
    Instr_Stats_Report.Instruction_Hit = 0;
    Instr_Stats_Report.Instruction_Miss = 0;
    Instr_Stats_Report.Instruction_Read_Access = 0;
    Instr_Stats_Report.Instruction_Write_Access = 0;
    Instr_Stats_Report.Instr_Hit_Ratio = 0.0;

    return true;    
}

unsigned int Selection_Menu()
{       
    unsigned int mode = 3;

    printf("\033[32;4;1m3. Display Mode Selection:\n");
    printf("|  Mode  |                                Description                                  |\n");
    printf("|   0    |                         Cache content & Statistics                          |\n");
    printf("|   1    |                 Mode 0 information & message between L1/L2                  |\n"); 
    do {
        printf("\033[0m\033[32m\tPLEASE SELECT A MODE: ");
        scanf("%u", &mode);
        printf("\tTHE ENTERED MODE IS: %u\033[0m\n", mode);
    } while (mode > 1);    
    return mode;
}

FILE* Open_Trace_File(char* Trace_File)
{        
    FILE *fd = fopen(Trace_File, "r");    
    return fd;
}

bool Read_and_Run_Trace_File(FILE* fd)
{
    char one_trace_line[MAX_TRACES];
    char* comments = NULL;
    unsigned int tmp_operation;    
    uint32_t address;
    
    // Initialize expected results
    Expected_Results_Typedef expected;
    memset(&expected, 0, sizeof(Expected_Results_Typedef));
    expected.has_expected = 0;

    // Parse metadata from beginning of file
    while (fgets(one_trace_line, MAX_TRACES, fd) != NULL)
    {
        if (strstr(one_trace_line, "# TEST_NAME:")) {
            sscanf(one_trace_line, "# TEST_NAME: %[^\n]", expected.test_name);
            expected.has_expected = 1;
        }
        else if (strstr(one_trace_line, "# EXPECTED_DATA_READ:")) {
            sscanf(one_trace_line, "# EXPECTED_DATA_READ: %u", &expected.expected_data_read);
        }
        else if (strstr(one_trace_line, "# EXPECTED_DATA_WRITE:")) {
            sscanf(one_trace_line, "# EXPECTED_DATA_WRITE: %u", &expected.expected_data_write);
        }
        else if (strstr(one_trace_line, "# EXPECTED_DATA_HIT:")) {
            sscanf(one_trace_line, "# EXPECTED_DATA_HIT: %u", &expected.expected_data_hit);
        }
        else if (strstr(one_trace_line, "# EXPECTED_DATA_MISS:")) {
            sscanf(one_trace_line, "# EXPECTED_DATA_MISS: %u", &expected.expected_data_miss);
        }
        else if (strstr(one_trace_line, "# EXPECTED_WRITE_BACK:")) {
            sscanf(one_trace_line, "# EXPECTED_WRITE_BACK: %u", &expected.expected_write_back);
        }
        else if (strstr(one_trace_line, "# EXPECTED_INSTR_READ:")) {
            sscanf(one_trace_line, "# EXPECTED_INSTR_READ: %u", &expected.expected_instr_read);
        }
        else if (strstr(one_trace_line, "# EXPECTED_INSTR_WRITE:")) {
            sscanf(one_trace_line, "# EXPECTED_INSTR_WRITE: %u", &expected.expected_instr_write);
        }
        else if (strstr(one_trace_line, "# EXPECTED_INSTR_HIT:")) {
            sscanf(one_trace_line, "# EXPECTED_INSTR_HIT: %u", &expected.expected_instr_hit);
        }
        else if (strstr(one_trace_line, "# EXPECTED_INSTR_MISS:")) {
            sscanf(one_trace_line, "# EXPECTED_INSTR_MISS: %u", &expected.expected_instr_miss);
        }
        else if (strstr(one_trace_line, "# EXPECTED_DATA_HIT_RATIO:")) {
            sscanf(one_trace_line, "# EXPECTED_DATA_HIT_RATIO: %f", &expected.expected_data_hit_ratio);
        }
        else if (strstr(one_trace_line, "# EXPECTED_INSTR_HIT_RATIO:")) {
            sscanf(one_trace_line, "# EXPECTED_INSTR_HIT_RATIO: %f", &expected.expected_instr_hit_ratio);
        }
        else if (one_trace_line[0] != '#') {
            // End of metadata, this is first operation line
            // Process it here
            sscanf(one_trace_line, "%u %x %c", &tmp_operation, &address, comments);
            if (tmp_operation <= 9) {
                switch (tmp_operation)
                {
                case READ:                
                    Data_Cache_Read(address);                          
                    break;
                
                case WRITE:
                    Data_Cache_Write(address);
                    break;
                
                case FETCH:
                    Instruction_Cache_Fetch(address);                             
                    break;

                case EVICT:                
                    L2_Evict_Command_to_L1(address);                                
                    break;                

                case RESET_AND_CLEAR:
                    Reset_And_Clear_Cache();                                              
                    break;

                case PRINT_LOG:                
                    Print_Content_And_State();
                    break;

                default:
                    break;
                }
            }
            break;  // Exit metadata parsing
        }
    }

    // Continue processing rest of file
    while (fgets(one_trace_line, MAX_TRACES, fd) != NULL)
    {          
        sscanf(one_trace_line, "%u %x %c", &tmp_operation, &address, comments);        
        if (one_trace_line[0]=='#'||!strcmp(one_trace_line, "\n")||!strcmp(one_trace_line, " ")||!strcmp(one_trace_line, "/")||!strcmp(one_trace_line, "*")||!strcmp(one_trace_line, "=")||(tmp_operation > 9)) 
            __asm__("nop");
        else
        {            
            switch (tmp_operation)
            {
            case READ:                
                Data_Cache_Read(address);                          
                break;
            
            case WRITE:
                Data_Cache_Write(address);
                break;
            
            case FETCH:
                Instruction_Cache_Fetch(address);                             
                break;

            case EVICT:                
                L2_Evict_Command_to_L1(address);                                
                break;                

            case RESET_AND_CLEAR:
                Reset_And_Clear_Cache();                                              
                break;

            case PRINT_LOG:                
                Print_Content_And_State();
                break;

            default:
                printf("\033[1;31mERROR: Invalid operation!\033[1;0m\n");
                break;
            }
        }
    }
    
    // Calculate hit ratios
    if (Data_Stats_Report.Data_Hit + Data_Stats_Report.Data_Miss > 0)
        Data_Stats_Report.Data_Hit_Ratio = (float)Data_Stats_Report.Data_Hit / 
            (Data_Stats_Report.Data_Hit + Data_Stats_Report.Data_Miss);
    
    if (Instr_Stats_Report.Instruction_Hit + Instr_Stats_Report.Instruction_Miss > 0)
        Instr_Stats_Report.Instr_Hit_Ratio = (float)Instr_Stats_Report.Instruction_Hit / 
            (Instr_Stats_Report.Instruction_Hit + Instr_Stats_Report.Instruction_Miss);
    
    // Auto-verification if metadata exists
    if (expected.has_expected) {
        Print_Expected_Results(&expected);
        Verify_Results(&expected);
    }
    
    return true;
}

/*======================================================================*/
/* CACHE OPERATIONS */
/*======================================================================*/

bool Data_Cache_Read(unsigned int address)
{
    uint16_t Tag = address >> (SET_BIT + BYTE_BIT);
    uint16_t Set = (address & SET_MASK) >> BYTE_BIT;
    int Selected_Cache_Way = -1;

    Data_Stats_Report.Data_Read_Access++;
    
    Selected_Cache_Way = Data_Match_Find(Tag, Set);
    
    if (Selected_Cache_Way >= 0 && Data_Cache[Selected_Cache_Way][Set].Valid)
    {
        Data_Stats_Report.Data_Hit++;
        Data_Cache[Selected_Cache_Way][Set].address = address;
        Data_LRU_State_Update(Selected_Cache_Way, Set);
        
        if ((Mode > 0) && (Hit_Show == 1)) 
            printf("\033[33m[READ ACCESS %6u] L1(DATA)  READ HIT <0x%08x>\033[0m\n", 
                   Data_Stats_Report.Data_Read_Access, address);
        return false;
    }
    
    Data_Stats_Report.Data_Miss++;
    
    Selected_Cache_Way = -1;
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (Data_Cache[i][Set].tag == TAG_INVALID || Data_Cache[i][Set].Valid == 0)
        {
            Selected_Cache_Way = i;
            break;
        }
    }
    
    if (Selected_Cache_Way < 0)
    {
        Selected_Cache_Way = Data_LRU_Smallest_Find(Set);
        
        if (Selected_Cache_Way < 0)
        {
            printf("\033[1;31mERROR: READ - Cannot find LRU victim!\033[1;0m\n");
            return true;
        }
        
        if (Data_Cache[Selected_Cache_Way][Set].Dirty)
        {
            if (Mode > 0)
                printf("\033[33;4m[READ_ ACCESS %6u] L1(DATA)  READ MISS  - Write to L2 <0x%08x> - Read from L2 <0x%08x>\033[0m\n",
                       Data_Stats_Report.Data_Read_Access, 
                       Data_Cache[Selected_Cache_Way][Set].address, 
                       address);
            Data_Stats_Report.Write_Back++;
        }
        else
        {
            if (Mode > 0)
                printf("\033[33;4m[READ_ ACCESS %6u] L1(DATA)  READ MISS  - L1 evict <0x%08x> - Read from L2 <0x%08x>\033[0m\n",
                       Data_Stats_Report.Data_Read_Access, 
                       Data_Cache[Selected_Cache_Way][Set].address, 
                       address);
        }
    }
    else
    {
        if (Mode > 0)
            printf("\033[33;4m[READ_ ACCESS %6u] L1(DATA)  READ MISS  - Read from L2 <0x%08x>\033[0m\n",
                   Data_Stats_Report.Data_Read_Access, address);
    }
    
    Data_Cache[Selected_Cache_Way][Set].tag = Tag;
    Data_Cache[Selected_Cache_Way][Set].set = Set;
    Data_Cache[Selected_Cache_Way][Set].Valid = 1;
    Data_Cache[Selected_Cache_Way][Set].Dirty = 0;
    Data_Cache[Selected_Cache_Way][Set].First_Write_Done = 0;
    Data_Cache[Selected_Cache_Way][Set].address = address;
    Data_LRU_State_Update(Selected_Cache_Way, Set);
    
    return false;
}

bool Data_Cache_Write(unsigned int address)
{
    uint16_t Tag = address >> (SET_BIT + BYTE_BIT);
    uint16_t Set = (address & SET_MASK) >> BYTE_BIT;
    int Selected_Cache_Way = -1;

    Data_Stats_Report.Data_Write_Access++;
    
    Selected_Cache_Way = Data_Match_Find(Tag, Set);
    
    if (Selected_Cache_Way >= 0 && Data_Cache[Selected_Cache_Way][Set].Valid)
    {
        Data_Stats_Report.Data_Hit++;
        
        if (Data_Cache[Selected_Cache_Way][Set].First_Write_Done == 0)
        {
            if (Mode > 0)
                printf("\033[33;4m[WRITE ACCESS %6u] L1(DATA)  WRITE HIT  - Write to L2 <0x%08x> (First Write - Write Through)\033[0m\n",
                       Data_Stats_Report.Data_Write_Access, address);
            
            Data_Cache[Selected_Cache_Way][Set].First_Write_Done = 1;
            Data_Cache[Selected_Cache_Way][Set].Dirty = 0;
        }
        else
        {
            Data_Cache[Selected_Cache_Way][Set].Dirty = 1;
            
            if ((Mode > 0) && (Hit_Show == 1))
                printf("\033[33m[WRITE ACCESS %6u] L1(DATA)  WRITE HIT <0x%08x>\033[0m\n",
                       Data_Stats_Report.Data_Write_Access, address);
        }
        
        Data_Cache[Selected_Cache_Way][Set].address = address;
        Data_LRU_State_Update(Selected_Cache_Way, Set);
        return false;
    }
    
    Data_Stats_Report.Data_Miss++;
    
    Selected_Cache_Way = -1;
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (Data_Cache[i][Set].tag == TAG_INVALID || Data_Cache[i][Set].Valid == 0)
        {
            Selected_Cache_Way = i;
            break;
        }
    }
    
    if (Selected_Cache_Way < 0)
    {
        Selected_Cache_Way = Data_LRU_Smallest_Find(Set);
        
        if (Selected_Cache_Way < 0)
        {
            printf("\033[31;4mERROR: WRITE - Cannot find LRU victim!\033[0m\n");
            return true;
        }
        
        if (Data_Cache[Selected_Cache_Way][Set].Dirty)
        {
            if (Mode > 0)
                printf("\033[33;4m[WRITE ACCESS %6u] L1(DATA)  WRITE MISS - Write to L2 <0x%08x> - Read for Ownership from L2 <0x%08x>\033[0m\n",
                       Data_Stats_Report.Data_Write_Access, 
                       Data_Cache[Selected_Cache_Way][Set].address, 
                       address);
            Data_Stats_Report.Write_Back++;
        }
        else
        {
            if (Mode > 0)
                printf("\033[33;4m[WRITE ACCESS %6u] L1(DATA)  WRITE MISS - L1 evict <0x%08x> - Read for Ownership from L2 <0x%08x>\033[0m\n",
                       Data_Stats_Report.Data_Write_Access, 
                       Data_Cache[Selected_Cache_Way][Set].address, 
                       address);
        }
    }
    else
    {
        if (Mode > 0)
            printf("\033[33;4m[WRITE ACCESS %6u] L1(DATA)  WRITE MISS - Read for Ownership from L2 <0x%08x>\033[0m\n",
                   Data_Stats_Report.Data_Write_Access, address);
    }
    
    Data_Cache[Selected_Cache_Way][Set].tag = Tag;
    Data_Cache[Selected_Cache_Way][Set].set = Set;
    Data_Cache[Selected_Cache_Way][Set].Valid = 1;
    Data_Cache[Selected_Cache_Way][Set].Dirty = 1;
    Data_Cache[Selected_Cache_Way][Set].First_Write_Done = 1;
    Data_Cache[Selected_Cache_Way][Set].address = address;
    Data_LRU_State_Update(Selected_Cache_Way, Set);
    
    return false;
}

bool Instruction_Cache_Fetch(unsigned int address)
{
    uint16_t Tag = address >> (SET_BIT + BYTE_BIT);
    uint16_t Set = (address & SET_MASK) >> BYTE_BIT;
    int Selected_Cache_Way = -1;

    Instr_Stats_Report.Instruction_Read_Access++;
    
    Selected_Cache_Way = Instruction_Match_Find(Tag, Set);
    
    if (Selected_Cache_Way >= 0 && Instr_Cache[Selected_Cache_Way][Set].Valid)
    {
        Instr_Stats_Report.Instruction_Hit++;
        Instr_Cache[Selected_Cache_Way][Set].address = address;
        Instruction_LRU_State_Update(Selected_Cache_Way, Set);
        
        if ((Mode > 0) && (Hit_Show == 1))
            printf("\033[33m[READ_ ACCESS %6u] L1(INSTR) READ HIT <0x%08x>\033[0m\n",
                   Instr_Stats_Report.Instruction_Read_Access, address);
        return false;
    }
    
    Instr_Stats_Report.Instruction_Miss++;
    
    Selected_Cache_Way = -1;
    for (uint8_t i = 0; i < INSTR_WAYS; i++)
    {
        if (Instr_Cache[i][Set].tag == TAG_INVALID || Instr_Cache[i][Set].Valid == 0)
        {
            Selected_Cache_Way = i;
            break;
        }
    }
    
    if (Selected_Cache_Way < 0)
    {
        Selected_Cache_Way = Instruction_LRU_Smallest_Find(Set);
        
        if (Selected_Cache_Way < 0)
        {
            printf("ERROR: FETCH - Cannot find LRU victim!\n");
            return true;
        }
        
        if (Mode > 0)
            printf("\033[33;4m[READ_ ACCESS %6u] L1(INSTR) READ MISS  - L1 evict <0x%08x> - Read from L2 <0x%08x>\033[0m\n",
                   Instr_Stats_Report.Instruction_Read_Access, 
                   Instr_Cache[Selected_Cache_Way][Set].address, 
                   address);
    }
    else
    {
        if (Mode > 0)
            printf("\033[33;4m[READ_ ACCESS %6u] L1(INSTR) READ MISS  - Read from L2 <0x%08x>\033[0m\n",
                   Instr_Stats_Report.Instruction_Read_Access, address);
    }
    
    Instr_Cache[Selected_Cache_Way][Set].tag = Tag;
    Instr_Cache[Selected_Cache_Way][Set].set = Set;
    Instr_Cache[Selected_Cache_Way][Set].Valid = 1;
    Instr_Cache[Selected_Cache_Way][Set].Dirty = 0;
    Instr_Cache[Selected_Cache_Way][Set].address = address;
    Instruction_LRU_State_Update(Selected_Cache_Way, Set);
    
    return false;
}

bool L2_Evict_Command_to_L1(unsigned int address)
{
    uint16_t Tag = address >> (SET_BIT + BYTE_BIT);
    uint16_t Set = (address & SET_MASK) >> BYTE_BIT;
    bool found = false;

    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (Data_Cache[i][Set].tag == Tag && Data_Cache[i][Set].Valid)
        {
            found = true;
            
            if (Data_Cache[i][Set].Dirty)
            {
                if (Mode > 0)
                    printf("\033[33;4mEVICTION FROM L2 - Write to L2 <0x%08x>\033[0m\n", address);
                Data_Stats_Report.Write_Back++;
            }
            
            Data_Cache[i][Set].Valid = 0;
            Data_Cache[i][Set].Dirty = 0;
            Data_Cache[i][Set].First_Write_Done = 0;
            break;
        }
    }
    
    if (!found)
    {
        for (uint8_t i = 0; i < INSTR_WAYS; i++)
        {
            if (Instr_Cache[i][Set].tag == Tag && Instr_Cache[i][Set].Valid)
            {
                found = true;
                Instr_Cache[i][Set].Valid = 0;
                break;
            }
        }
    }
    
    return false;
}

bool Print_Content_And_State()
{
    uint8_t Valid_in_Set = 0;

    Data_Stats_Report.Data_Hit_Ratio = (float)((Data_Stats_Report.Data_Hit*1.0)/(Data_Stats_Report.Data_Miss + Data_Stats_Report.Data_Hit));
    Instr_Stats_Report.Instr_Hit_Ratio = (float)((Instr_Stats_Report.Instruction_Hit*1.0)/(Instr_Stats_Report.Instruction_Miss + Instr_Stats_Report.Instruction_Hit));    
    
    if (Mode > 0) printf("\033[33m==============================================================================================================\033[0m\n");
    printf("\033[36m==============================================================================================================\033[0m\n");
    printf("\033[36m\t\t\t\t\033[4;1mL1 CACHE SUMMARY AND STATISTICS:\033[0m\n");
    
    printf("\033[36m\033[4;1m1. DATA CACHE CONTENT:\n\033[0m\n");
    for (uint16_t i = 0; i < NUM_SETS; i++)
    {
        for (uint8_t j = 0; j < DATA_WAYS; j++)
        {
            if (Data_Cache[j][i].Valid == 1)
            {
                if (Valid_in_Set == 0)
                {
                    printf("\033[36m\033[4mSet Index: %u\033[0m\n", i);
                    Valid_in_Set = 1;
                }
                printf("\033[36mWay Index: %u || Address: 0x%08x || Tag: %04u || Set: %05u || LRU: %1d || Valid: %u || Dirty: %d\033[0m\n", 
                j, Data_Cache[j][i].address, Data_Cache[j][i].tag, Data_Cache[j][i].set, Data_Cache[j][i].LRU_State, Data_Cache[j][i].Valid, Data_Cache[j][i].Dirty);
            }            
        }
        Valid_in_Set = 0;
    }
    printf("\033[36m--------------------------------------------------------------------------------------------------------------\033[0m\n");

    printf("\033[36m\033[4;1m2. INSTRUCTION CACHE CONTENT:\n\033[0m\n");
    for (uint16_t i = 0; i < NUM_SETS; i++)
    {
        for (uint8_t j = 0; j < INSTR_WAYS; j++)
        {
            if (Instr_Cache[j][i].Valid == 1)
            {
                if (Valid_in_Set == 0)
                {
                    printf("\033[36m\033[4mSet Index: %u\033[0m\n", i);
                    Valid_in_Set = 1;
                }
                printf("\033[36mWay Index: %u || Address: 0x%08x || Tag: %04u || Set: %05u || LRU: %1d || Valid: %u\033[0m\n", 
                j, Instr_Cache[j][i].address, Instr_Cache[j][i].tag, Instr_Cache[j][i].set, Instr_Cache[j][i].LRU_State, Instr_Cache[j][i].Valid);
            }            
        }
        Valid_in_Set = 0;
    }
    printf("\033[36m--------------------------------------------------------------------------------------------------------------\033[0m\n");

    printf("\033[36m\033[4;1m3. L1 CACHE STATISTICS:\n\033[0m\n");
    printf("\033[36m\033[1ma. DATA CACHE:\033[0m\n");
    if (Data_Stats_Report.Data_Miss == 0)
    {
        printf("\t\033[36mNo operation was executed on Data Cache!\033[0m\n");        
    }
    else
    {
        printf("\033[36m\t+Data Cache Read Accesses: %u\n\t+Data Cache Write Accesses: %u\n\t+Data Cache Write Backs: %u\n\t+Data Cache Hits: %u\n\t+Data Cache Misses: %u\n\t+Data Cache Hit Ratio: %1.4f\n\033[0m\n", 
        Data_Stats_Report.Data_Read_Access, Data_Stats_Report.Data_Write_Access, Data_Stats_Report.Write_Back, Data_Stats_Report.Data_Hit, Data_Stats_Report.Data_Miss, Data_Stats_Report.Data_Hit_Ratio);
    }
    printf("\033[36m\033[1mb. INSTRUCTION CACHE:\033[0m\n");
    if (Instr_Stats_Report.Instruction_Miss == 0)
    {
        printf("\033[36m\tNo operation was executed on Instruction Cache!\033[0m\n");        
    }
    else
    {
        printf("\033[36m\t+Instruction Cache Read Accesses: %u\n\t+Instruction Cache Write Accesses: %u\n\t+Instruction Cache Hits: %u\n\t+Instruction Cache Misses: %u\n\t+Instruction Cache Hit Ratio: %1.4f\n\033[0m\n", 
        Instr_Stats_Report.Instruction_Read_Access, Instr_Stats_Report.Instruction_Write_Access, Instr_Stats_Report.Instruction_Hit, Instr_Stats_Report.Instruction_Miss, Instr_Stats_Report.Instr_Hit_Ratio);
    }
    printf("\033[36m==============================================================================================================\033[0m\n");
    return false;
}

/*======================================================================*/
/* SUPPORT FUNCTIONS */
/*======================================================================*/

int Data_Match_Find(unsigned int Input_Tag, unsigned int Input_Set)
{        
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (Data_Cache[i][Input_Set].tag == Input_Tag) 
            return i;               
    }
    return -1;
}

int Instruction_Match_Find(unsigned int Input_Tag, unsigned int Input_Set)
{
    for (uint8_t i = 0; i < INSTR_WAYS; i++)
    {
        if (Instr_Cache[i][Input_Set].tag == Input_Tag) 
            return i;                
    }
    return -1;
}

void Data_LRU_State_Update(unsigned int Set_Way, unsigned int Cache_Set)
{
    uint8_t Current_LRU = Data_Cache[Set_Way][Cache_Set].LRU_State;
    
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (i != Set_Way && Data_Cache[i][Cache_Set].Valid)
        {
            if (Data_Cache[i][Cache_Set].LRU_State > Current_LRU)
            {
                Data_Cache[i][Cache_Set].LRU_State--;
            }
        }
    }
    
    Data_Cache[Set_Way][Cache_Set].LRU_State = DATA_LRU_MAX;
}

void Instruction_LRU_State_Update(unsigned int Set_Way, unsigned int Cache_Set)
{
    uint8_t Current_LRU = Instr_Cache[Set_Way][Cache_Set].LRU_State;

    for (uint8_t i = 0; i < INSTR_WAYS; i++)
    {
        if (i != Set_Way && Instr_Cache[i][Cache_Set].Valid)
        {
            if (Instr_Cache[i][Cache_Set].LRU_State > Current_LRU)
            {
                Instr_Cache[i][Cache_Set].LRU_State--;
            }
        }
    }
    
    Instr_Cache[Set_Way][Cache_Set].LRU_State = INSTR_LRU_MAX;
}

int Data_LRU_Smallest_Find(uint16_t Set_Index)
{
    for (uint8_t i = 0; i < DATA_WAYS; i++)
    {
        if (Data_Cache[i][Set_Index].LRU_State == DATA_LRU_MIN) 
            return i;               
    }
    return -1;
}

int Instruction_LRU_Smallest_Find(uint16_t Set_Index)
{
    for (uint8_t i = 0; i < INSTR_WAYS; i++)
    {
        if (Instr_Cache[i][Set_Index].LRU_State == INSTR_LRU_MIN) 
            return i;              
    }
    return -1;
}

/*======================================================================*/
/* AUTO-VERIFICATION FUNCTIONS */
/*======================================================================*/

void Print_Expected_Results(Expected_Results_Typedef* expected)
{
    printf("\n\033[35m╔════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[35m║              📝 EXPECTED RESULTS - KẾT QUẢ MONG ĐỢI            ║\033[0m\n");
    printf("\033[35m╠════════════════════════════════════════════════════════════════╣\033[0m\n");
    printf("\033[35m║ Test Name: %-50s ║\033[0m\n", expected->test_name);
    printf("\033[35m╠════════════════════════════════════════════════════════════════╣\033[0m\n");
    printf("\033[35m║ DATA CACHE:                                                    ║\033[0m\n");
    printf("\033[35m║   - Expected Reads:       %-8u                           ║\033[0m\n", expected->expected_data_read);
    printf("\033[35m║   - Expected Writes:      %-8u                           ║\033[0m\n", expected->expected_data_write);
    printf("\033[35m║   - Expected Hits:        %-8u                           ║\033[0m\n", expected->expected_data_hit);
    printf("\033[35m║   - Expected Misses:      %-8u                           ║\033[0m\n", expected->expected_data_miss);
    printf("\033[35m║   - Expected Write Backs: %-8u                           ║\033[0m\n", expected->expected_write_back);
    printf("\033[35m║   - Expected Hit Ratio:   %.4f                              ║\033[0m\n", expected->expected_data_hit_ratio);
    printf("\033[35m╠════════════════════════════════════════════════════════════════╣\033[0m\n");
    printf("\033[35m║ INSTRUCTION CACHE:                                             ║\033[0m\n");
    printf("\033[35m║   - Expected Reads:       %-8u                           ║\033[0m\n", expected->expected_instr_read);
    printf("\033[35m║   - Expected Writes:      %-8u (MUST be 0)               ║\033[0m\n", expected->expected_instr_write);
    printf("\033[35m║   - Expected Hits:        %-8u                           ║\033[0m\n", expected->expected_instr_hit);
    printf("\033[35m║   - Expected Misses:      %-8u                           ║\033[0m\n", expected->expected_instr_miss);
    printf("\033[35m║   - Expected Hit Ratio:   %.4f                              ║\033[0m\n", expected->expected_instr_hit_ratio);
    printf("\033[35m╚════════════════════════════════════════════════════════════════╝\033[0m\n");
}

bool Verify_Results(Expected_Results_Typedef* expected)
{
    bool pass = true;
    printf("\n\033[36m╔════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[36m║              ✅ VERIFICATION - KIỂM TRA KẾT QUẢ                ║\033[0m\n");
    printf("\033[36m╚════════════════════════════════════════════════════════════════╝\033[0m\n");
    
    // Data Cache Reads
    if (Data_Stats_Report.Data_Read_Access != expected->expected_data_read) {
        printf("\033[31m❌ FAIL: Data Reads - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_data_read, Data_Stats_Report.Data_Read_Access);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Data Reads = %u\033[0m\n", Data_Stats_Report.Data_Read_Access);
    }
    
    // Data Cache Writes
    if (Data_Stats_Report.Data_Write_Access != expected->expected_data_write) {
        printf("\033[31m❌ FAIL: Data Writes - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_data_write, Data_Stats_Report.Data_Write_Access);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Data Writes = %u\033[0m\n", Data_Stats_Report.Data_Write_Access);
    }
    
    // Data Cache Hits
    if (Data_Stats_Report.Data_Hit != expected->expected_data_hit) {
        printf("\033[31m❌ FAIL: Data Hits - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_data_hit, Data_Stats_Report.Data_Hit);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Data Hits = %u\033[0m\n", Data_Stats_Report.Data_Hit);
    }
    
    // Data Cache Misses
    if (Data_Stats_Report.Data_Miss != expected->expected_data_miss) {
        printf("\033[31m❌ FAIL: Data Misses - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_data_miss, Data_Stats_Report.Data_Miss);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Data Misses = %u\033[0m\n", Data_Stats_Report.Data_Miss);
    }
    
    // Write Backs
    if (Data_Stats_Report.Write_Back != expected->expected_write_back) {
        printf("\033[31m❌ FAIL: Write Backs - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_write_back, Data_Stats_Report.Write_Back);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Write Backs = %u\033[0m\n", Data_Stats_Report.Write_Back);
    }
    
    // Data Hit Ratio (with tolerance)
    float ratio_diff = fabs(Data_Stats_Report.Data_Hit_Ratio - expected->expected_data_hit_ratio);
    if (ratio_diff > 0.0001) {
        printf("\033[31m❌ FAIL: Data Hit Ratio - Expected: %.4f, Got: %.4f\033[0m\n", 
               expected->expected_data_hit_ratio, Data_Stats_Report.Data_Hit_Ratio);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Data Hit Ratio = %.4f\033[0m\n", Data_Stats_Report.Data_Hit_Ratio);
    }
    
    // Instruction Cache Reads
    if (Instr_Stats_Report.Instruction_Read_Access != expected->expected_instr_read) {
        printf("\033[31m❌ FAIL: Instr Reads - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_instr_read, Instr_Stats_Report.Instruction_Read_Access);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Instr Reads = %u\033[0m\n", Instr_Stats_Report.Instruction_Read_Access);
    }
    
    // Instruction Cache Writes (MUST be 0)
    if (Instr_Stats_Report.Instruction_Write_Access != expected->expected_instr_write) {
        printf("\033[31m❌ FAIL: Instr Writes - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_instr_write, Instr_Stats_Report.Instruction_Write_Access);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Instr Writes = %u (Correct - Instructions never written)\033[0m\n", 
               Instr_Stats_Report.Instruction_Write_Access);
    }
    
    // Instruction Cache Hits
    if (Instr_Stats_Report.Instruction_Hit != expected->expected_instr_hit) {
        printf("\033[31m❌ FAIL: Instr Hits - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_instr_hit, Instr_Stats_Report.Instruction_Hit);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Instr Hits = %u\033[0m\n", Instr_Stats_Report.Instruction_Hit);
    }
    
    // Instruction Cache Misses
    if (Instr_Stats_Report.Instruction_Miss != expected->expected_instr_miss) {
        printf("\033[31m❌ FAIL: Instr Misses - Expected: %u, Got: %u\033[0m\n", 
               expected->expected_instr_miss, Instr_Stats_Report.Instruction_Miss);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Instr Misses = %u\033[0m\n", Instr_Stats_Report.Instruction_Miss);
    }
    
    // Instruction Hit Ratio (with tolerance)
    float instr_ratio_diff = fabs(Instr_Stats_Report.Instr_Hit_Ratio - expected->expected_instr_hit_ratio);
    if (instr_ratio_diff > 0.0001) {
        printf("\033[31m❌ FAIL: Instr Hit Ratio - Expected: %.4f, Got: %.4f\033[0m\n", 
               expected->expected_instr_hit_ratio, Instr_Stats_Report.Instr_Hit_Ratio);
        pass = false;
    } else {
        printf("\033[32m✅ PASS: Instr Hit Ratio = %.4f\033[0m\n", Instr_Stats_Report.Instr_Hit_Ratio);
    }
    
    // Final verdict
    printf("\n");
    if (pass) {
        printf("\033[42;1m                    🎉 ALL TESTS PASSED! 🎉                    \033[0m\n");
    } else {
        printf("\033[41;1m                    ⚠️  SOME TESTS FAILED  ⚠️                   \033[0m\n");
    }
    printf("\n");
    
    return pass;
}

/* END User function */

/*======================================================================*/
/*
COMPLETE CACHE SIMULATOR WITH AUTO-VERIFICATION

FEATURES:
✅ Split L1 Cache (4-way Data, 2-way Instruction)
✅ Write-allocate, Write-back (first write = write-through)
✅ LRU replacement policy
✅ Inclusive hierarchy with L2
✅ Mode 0 (stats only) and Mode 1 (+ L2 messages)
✅ Operations: 0-3 (read/write/fetch/evict), 8 (reset), 9 (print)
✅ Auto-verification with metadata parsing
✅ Comprehensive statistics tracking
✅ Color-coded output for easy debugging

USAGE:
    gcc -o cache Cache_modified.c -lm
    ./cache trace_file.txt [mode]

TRACE FILE FORMAT:
    # TEST_NAME: Your Test Name
    # EXPECTED_DATA_READ: 2
    # EXPECTED_DATA_WRITE: 1
    # ... (all expected values)
    0 0x408040    # operations
    1 0x408100
    9             # print stats
    
TESTING CHECKLIST:
□ First write triggers write-through message
□ Subsequent writes set dirty bit only
□ LRU replacement works correctly
□ Dirty line eviction triggers write-back
□ L2 eviction invalidates L1 lines
□ Mode 0 shows stats only (no L2 messages)
□ Mode 1 shows all L2 communication
□ Operation 9 prints only valid lines
□ Operation 8 resets cache and stats
□ Instruction cache writes always = 0
□ Statistics are accurate
□ Auto-verification passes all tests
*/
/*======================================================================*/