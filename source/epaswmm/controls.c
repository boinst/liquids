//-----------------------------------------------------------------------------
//   controls.c
//
//   Project:  EPA SWMM5
//   Version:  5.0
//   Date:     6/19/07  (Build 5.0.010)
//             2/4/08   (Build 5.0.012)
//             3/11/08  (Build 5.0.013)
//             1/21/09  (Build 5.0.014)
//             07/30/10 (Build 5.0.019)
//   Author:   L. Rossman
//
//   Rule-based controls functions.
//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE

#include <malloc.h>
#include <math.h>
#include "headers.h"

//-----------------------------------------------------------------------------
//  Constants
//-----------------------------------------------------------------------------
enum RuleState   {r_RULE, r_IF, r_AND, r_OR, r_THEN, r_ELSE, r_PRIORITY,
                  r_ERROR};
enum RuleObject  {r_NODE, r_LINK, r_PUMP, r_ORIFICE, r_WEIR, r_OUTLET, r_SIMULATION};
enum RuleAttrib  {r_DEPTH, r_HEAD, r_INFLOW, r_FLOW, r_STATUS, r_SETTING,
                  r_TIME, r_DATE, r_CLOCKTIME, r_DAY, r_MONTH};                //(5.0.014 - LR)
enum RuleOperand {EQ, NE, LT, LE, GT, GE};
enum RuleSetting {r_CURVE, r_TIMESERIES, r_PID, r_NUMERIC};                    //(5.0.012 - LR)

static char* ObjectWords[] =
    {"NODE", "LINK", "PUMP", "ORIFICE", "WEIR", "OUTLET", "SIMULATION", NULL};
static char* AttribWords[] =
    {"DEPTH", "HEAD", "INFLOW", "FLOW", "STATUS", "SETTING",
     "TIME", "DATE", "CLOCKTIME", "DAY", "MONTH", NULL};                       //(5.0.014 - LR)
static char* OperandWords[] = {"=", "<>", "<", "<=", ">", ">=", NULL};
static char* StatusWords[]  = {"OFF", "ON", NULL};
static char* SettingTypeWords[] = {"CURVE", "TIMESERIES", "PID", NULL};        //(5.0.012 - LR)

//-----------------------------------------------------------------------------                  
// Data Structures
//-----------------------------------------------------------------------------                  
// Rule Premise Clause 
struct  TPremise
{
   int      type;
   int      node;
   int      link;
   int      attribute;
   int      operand;
   double   value;
   struct   TPremise *next;
};

// Rule Action Clause
struct  TAction              
{
   int     rule;
   int     link;
   int     attribute;
   int     curve;
   int     tseries;
   double  value;
   double  kp, ki, kd;                                                         //(5.0.012 - LR)
   double  e1, e2;                                                             //(5.0.012 - LR)
   struct  TAction *next;
};

// List of Control Actions
struct  TActionList          
{
   struct  TAction* action;
   struct  TActionList* next;
};

// Control Rule
struct  TRule
{
   char*    ID;                        // rule ID
   double   priority;                  // Priority level
   struct   TPremise* firstPremise;    // Pointer to first premise of rule
   struct   TPremise* lastPremise;     // Pointer to last premise of rule
   struct   TAction*  thenActions;     // Linked list of actions if true
   struct   TAction*  elseActions;     // Linked list of actions if false
};

//-----------------------------------------------------------------------------
//  Shared variables
//-----------------------------------------------------------------------------
struct TRule*       Rules;             // Array of control rules
struct TActionList* ActionList;        // Linked list of control actions
int    InputState;                     // State of rule interpreter
int    RuleCount;                      // Total number of rules
double ControlValue;                   // Value of controller variable
double SetPoint;                       // Value of controller setpoint         //(5.0.012 - LR)

//-----------------------------------------------------------------------------
//  External functions (declared in funcs.h)
//-----------------------------------------------------------------------------
//     controls_create
//     controls_delete
//     controls_addRuleClause
//     controls_evaluate

//-----------------------------------------------------------------------------
//  Local functions
//-----------------------------------------------------------------------------
int    addPremise(int r, int type, char* Tok[], int nToks);
int    addAction(int r, char* Tok[], int nToks);
int    evaluatePremise(struct TPremise* p, DateTime theDate, DateTime theTime,
                       DateTime elapsedTime, double tStep);
int    checkTimeValue(struct TPremise* p, double time1, double time2);
int    checkValue(struct TPremise* p, double x);
void   updateActionList(struct TAction* a);
int    executeActionList(DateTime currentTime);
void   clearActionList(void);
void   deleteActionList(void);
void   deleteRules(void);
int    findExactMatch(char *s, char *keyword[]);
int    setActionSetting(char* tok[], int nToks, int* curve, int* tseries,
       int* attrib, double* value);                                            //(5.0.012 - LR)
void   updateActionValue(struct TAction* a, DateTime currentTime, double dt);
double getPIDSetting(struct TAction* a, double dt);                            //(5.0.012 - LR)


//=============================================================================

int  controls_create(int n)
//
//  Input:   n = total number of control rules
//  Output:  returns error code
//  Purpose: creates an array of control rules.
//
{
   int r;
   ActionList = NULL;
   InputState = r_PRIORITY;
   RuleCount = n;
   if ( n == 0 ) return 0;
   Rules = (struct TRule *) calloc(RuleCount, sizeof(struct TRule));
   if (Rules == NULL) return ERR_MEMORY;
   for ( r=0; r<RuleCount; r++ )
   {
       Rules[r].ID = NULL;
       Rules[r].firstPremise = NULL;
       Rules[r].lastPremise = NULL;
       Rules[r].thenActions = NULL;
       Rules[r].elseActions = NULL;
       Rules[r].priority = 0.0;    
   }
   return 0;
}

//=============================================================================

void controls_delete(void)
//
//  Input:   none
//  Output:  none
//  Purpose: deletes all control rules.
//
{
   if ( RuleCount == 0 ) return;
   deleteActionList();
   deleteRules();
}

//=============================================================================

int  controls_addRuleClause(int r, int keyword, char* tok[], int nToks)
//
//  Input:   r = rule index
//           keyword = the clause's keyword code (IF, THEN, etc.)
//           tok = an array of string tokens that comprises the clause
//           nToks = number of tokens
//  Output:  returns an error  code
//  Purpose: addd a new clause to a control rule.
//
{
    switch (keyword)
    {
      case r_RULE:
        if ( Rules[r].ID == NULL )
            Rules[r].ID = project_findID(CONTROL, tok[1]);
        InputState = r_RULE;
        if ( nToks > 2 ) return ERR_RULE;                                      //(5.0.010 - LR)
        return 0;

      case r_IF:
        if ( InputState != r_RULE ) return ERR_RULE;
        InputState = r_IF;
        return addPremise(r, r_AND, tok, nToks);

      case r_AND:
        if ( InputState == r_IF ) return addPremise(r, r_AND, tok, nToks);
        else if ( InputState == r_THEN || InputState == r_ELSE )
            return addAction(r, tok, nToks);
        else return ERR_RULE;

      case r_OR:
        if ( InputState != r_IF ) return ERR_RULE;
        return addPremise(r, r_OR, tok, nToks);

      case r_THEN:
        if ( InputState != r_IF ) return ERR_RULE;
        InputState = r_THEN;
        return addAction(r, tok, nToks);

      case r_ELSE:
        if ( InputState != r_THEN ) return ERR_RULE;
        InputState = r_ELSE;
        return addAction(r, tok, nToks);

      case r_PRIORITY:
        if ( InputState != r_THEN && InputState != r_ELSE ) return ERR_RULE;
        InputState = r_PRIORITY;
        if ( !getDouble(tok[1], &Rules[r].priority) ) return ERR_NUMBER;
        if ( nToks > 2 ) return ERR_RULE;                                      //(5.0.010 - LR)
        return 0;
    }
    return 0;
}

//=============================================================================

int controls_evaluate(DateTime currentTime, DateTime elapsedTime, double tStep)
//
//  Input:   currentTime = current simulation date/time
//           elapsedTime = decimal days since start of simulation
//           tStep = simulation time step (days)                               //(5.0.013 - LR)
//  Output:  returns number of new actions taken
//  Purpose: evaluates all control rules at current time of the simulation.
//
{
    int    r;                          // control rule index
    int    result;                     // TRUE if rule premises satisfied
    struct TPremise* p;                // pointer to rule premise clause
    struct TAction*  a;                // pointer to rule action clause
    DateTime theDate = floor(currentTime);
    DateTime theTime = currentTime - floor(currentTime);

    // --- evaluate each rule
    if ( RuleCount == 0 ) return 0;
    clearActionList();
    for (r=0; r<RuleCount; r++)
    {
        // --- evaluate rule's premises
        result = TRUE;
        p = Rules[r].firstPremise;
        while (p)
        {
            if ( p->type == r_OR )
            {
                if ( result == FALSE )
                    result = evaluatePremise(p, theDate, theTime,
                                 elapsedTime, tStep);
            }
            else
            {
                if ( result == FALSE ) break;
                result = evaluatePremise(p, theDate, theTime, 
                             elapsedTime, tStep);
            }
            p = p->next;
        }    

        // --- if premises true, add THEN clauses to action list
        //     else add ELSE clauses to action list
        if ( result == TRUE ) a = Rules[r].thenActions;
        else                  a = Rules[r].elseActions;
        while (a)
        {
            updateActionValue(a, currentTime, tStep);                          //(5.0.012 - LR)
            updateActionList(a);
            a = a->next;
        }
    }

    // --- execute actions on action list
    if ( ActionList ) return executeActionList(currentTime);
    else return 0;
}

//=============================================================================

int  addPremise(int r, int type, char* tok[], int nToks)
//
//  Input:   r = control rule index
//           type = type of premise (IF, AND, OR)
//           tok = array of string tokens containing premise statement
//           nToks = number of string tokens
//  Output:  returns an error code
//  Purpose: adds a new premise to a control rule.
//
{
    int    node = -1;
    int    link = -1;
    int    obj, attrib, op, n;
    double value;
    struct TPremise* p;

    // --- check for proper number of tokens
    if ( nToks < 5 ) return ERR_ITEMS;

    // --- get object type
    obj = findmatch(tok[1], ObjectWords);
    if ( obj < 0 ) return error_setInpError(ERR_KEYWORD, tok[1]);

    // --- get object name
    n = 2;
    switch (obj)
    {
      case r_NODE:
        node = project_findObject(NODE, tok[n]);
        if ( node < 0 ) return error_setInpError(ERR_NAME, tok[n]);
        break;

      case r_LINK:
      case r_PUMP:
      case r_ORIFICE:
      case r_WEIR:
      case r_OUTLET:
        link = project_findObject(LINK, tok[n]);
        if ( link < 0 ) return error_setInpError(ERR_NAME, tok[n]);
        break;
      default: n = 1;
    }
    n++;

    // --- get attribute name
    attrib = findmatch(tok[n], AttribWords);
    if ( attrib < 0 ) return error_setInpError(ERR_KEYWORD, tok[n]);

    // --- check that property belongs to object type
    if ( obj == r_NODE ) switch (attrib)
    {
      case r_DEPTH:
      case r_HEAD:
      case r_INFLOW: break;
      default: return error_setInpError(ERR_KEYWORD, tok[n]);
    }
    else if ( obj == r_LINK ) switch (attrib)
    {
      case r_DEPTH:
      case r_FLOW: break;
      default: return error_setInpError(ERR_KEYWORD, tok[n]);
    }
    else if ( obj == r_PUMP ) switch (attrib)
    {
      case r_FLOW:
      case r_STATUS: break;
      default: return error_setInpError(ERR_KEYWORD, tok[n]);
    }
    else if ( obj == r_ORIFICE || obj == r_WEIR ||
              obj == r_OUTLET ) switch (attrib)                                //(5.0.010 - LR)
    {
      case r_SETTING: break;
      default: return error_setInpError(ERR_KEYWORD, tok[n]);
    }
    else switch (attrib)
    {
      case r_TIME:
      case r_DATE:
      case r_CLOCKTIME:                                                        //(5.0.014 - LR)
      case r_DAY:                                                              //(5.0.014 - LR)
      case r_MONTH: break;                                                     //(5.0.014 - LR)
      default: return error_setInpError(ERR_KEYWORD, tok[n]);
    }

    // --- get operand
    n++;
    op = findExactMatch(tok[n], OperandWords);
    if ( op < 0 ) return error_setInpError(ERR_KEYWORD, tok[n]);
    n++;
    if ( n >= nToks ) return error_setInpError(ERR_ITEMS, "");

    // --- get value
    switch (attrib)
    {
      case r_STATUS:
        value = findmatch(tok[n], StatusWords);
        if ( value < 0.0 ) return error_setInpError(ERR_KEYWORD, tok[n]);
        break;

      case r_TIME:
      case r_CLOCKTIME:
        if ( !datetime_strToTime(tok[n], &value) )
            return error_setInpError(ERR_DATETIME, tok[n]);
        break;

      case r_DATE:
        if ( !datetime_strToDate(tok[n], &value) )
            return error_setInpError(ERR_DATETIME, tok[n]);
        break;

      case r_DAY:
        if ( !getDouble(tok[n], &value) )                                      //(5.0.014 - LR)
            return error_setInpError(ERR_NUMBER, tok[n]);                      //(5.0.014 - LR)
        if ( value < 1.0 || value > 7.0 )                                      //(5.0.014 - LR)
             return error_setInpError(ERR_DATETIME, tok[n]);                   //(5.0.014 - LR)
        break;                                                                 //(5.0.014 - LR)


      case r_MONTH:                                                            //(5.0.014 - LR)
        if ( !getDouble(tok[n], &value) )                                      //(5.0.014 - LR)
            return error_setInpError(ERR_NUMBER, tok[n]);                      //(5.0.014 - LR)
        if ( value < 1.0 || value > 12.0 )                                     //(5.0.014 - LR)
             return error_setInpError(ERR_DATETIME, tok[n]);                   //(5.0.014 - LR)
        break;                                                                 //(5.0.014 - LR)
       
      default: if ( !getDouble(tok[n], &value) )
          return error_setInpError(ERR_NUMBER, tok[n]);
    }

    // --- check if another clause is on same line                             //(5.0.010 - LR)
    n++;                                                                       //(5.0.010 - LR) 
    if ( n < nToks && findmatch(tok[n], RuleKeyWords) >= 0 ) return ERR_RULE;  //(5.0.010 - LR)

    // --- create the premise object
    p = (struct TPremise *) malloc(sizeof(struct TPremise));
    if ( !p ) return ERR_MEMORY;
    p->type      = type;
    p->node      = node;
    p->link      = link;
    p->attribute = attrib;
    p->operand   = op;
    p->value     = value;
    p->next      = NULL;
    if ( Rules[r].firstPremise == NULL )
    {
        Rules[r].firstPremise = p;
    }
    else
    {
        Rules[r].lastPremise->next = p;
    }
    Rules[r].lastPremise = p;
    return 0;
}

//=============================================================================

int  addAction(int r, char* tok[], int nToks)
//
//  Input:   r = control rule index
//           tok = array of string tokens containing action statement
//           nToks = number of string tokens
//  Output:  returns an error code
//  Purpose: adds a new action to a control rule.
//
{
    int    obj, link, attrib;
    int    curve = -1, tseries = -1;
    int    n;                                                                  //(5.0.010 - LR)
    int    err;
    double values[] = {1.0, 0.0, 0.0};                                         //(5.0.012 - LR)

    struct TAction* a;

    // --- check for proper number of tokens
    if ( nToks < 6 ) return error_setInpError(ERR_ITEMS, "");

    // --- check for valid object type
    obj = findmatch(tok[1], ObjectWords);
    if ( obj != r_PUMP && obj != r_ORIFICE && obj != r_WEIR && obj != r_OUTLET )
        return error_setInpError(ERR_KEYWORD, tok[1]);

    // --- check that object name exists and is of correct type
    link = project_findObject(LINK, tok[2]);
    if ( link < 0 ) return error_setInpError(ERR_NAME, tok[2]);
    switch (obj)
    {
      case r_PUMP:
        if ( Link[link].type != PUMP )
            return error_setInpError(ERR_NAME, tok[2]);
        break;
      case r_ORIFICE:
        if ( Link[link].type != ORIFICE )
            return error_setInpError(ERR_NAME, tok[2]);
        break;
      case r_WEIR:
        if ( Link[link].type != WEIR )
            return error_setInpError(ERR_NAME, tok[2]);
        break;
      case r_OUTLET:
        if ( Link[link].type != OUTLET )
            return error_setInpError(ERR_NAME, tok[2]);
        break;
    }

    // --- check for valid attribute name
    attrib = findmatch(tok[3], AttribWords);
    if ( attrib < 0 ) return error_setInpError(ERR_KEYWORD, tok[3]);

    // --- get control action setting
    if ( obj == r_PUMP )
    {
        if ( attrib == r_STATUS )
        {
            values[0] = findmatch(tok[5], StatusWords);                        //(5.0.012 - LR)
            if ( values[0] < 0.0 )                                             //(5.0.012 - LR)
                return error_setInpError(ERR_KEYWORD, tok[5]);                 //(5.0.012 - LR)
        }
        else if ( attrib == r_SETTING )
        {
            err = setActionSetting(tok, nToks, &curve, &tseries,               //(5.0.012 - LR)
                                   &attrib, values);                           //(5.0.012 - LR)
            if ( err > 0 ) return err;
        }
        else return error_setInpError(ERR_KEYWORD, tok[3]);
    }

    else if ( obj == r_ORIFICE || obj == r_WEIR || obj == r_OUTLET )
    {
        if ( attrib == r_SETTING )                                             //(5.0.012 - LR)
        {                                                                      //(5.0.012 - LR)
           err = setActionSetting(tok, nToks, &curve, &tseries,                //(5.0.012 - LR)
                                  &attrib, values);                            //(5.0.012 - LR)
           if ( err > 0 ) return err;
           if (  attrib == r_SETTING                                           //(5.0.012 - LR)
           && (values[0] < 0.0 || values[0] > 1.0) )                           //(5.0.012 - LR)
               return error_setInpError(ERR_NUMBER, tok[5]);                   //(5.0.012 - LR)
        }                                                                      //(5.0.012 - LR)
        else return error_setInpError(ERR_KEYWORD, tok[3]);
    }
    else return error_setInpError(ERR_KEYWORD, tok[1]);

    // --- check if another clause is on same line                             //(5.0.010 - LR)
    n = 6;                                                                     //(5.0.010 - LR) 
    if ( curve >= 0 || tseries >= 0 ) n = 7;                                   //(5.0.010 - LR)
    if ( attrib == r_PID ) n = 9;                                              //(5.0.012 - LR)
    if ( n < nToks && findmatch(tok[n], RuleKeyWords) >= 0 ) return ERR_RULE;  //(5.0.010 - LR)

    // --- create the action object
    a = (struct TAction *) malloc(sizeof(struct TAction));
    if ( !a ) return ERR_MEMORY;
    a->rule      = r;
    a->link      = link;
    a->attribute = attrib;
    a->curve     = curve;
    a->tseries   = tseries;
    a->value     = values[0];                                                  //(5.0.012 - LR)
    if ( attrib == r_PID )                                                     //(5.0.012 - LR)
    {                                                                          //(5.0.012 - LR)
        a->kp = values[0];                                                     //(5.0.012 - LR)
        a->ki = values[1];                                                     //(5.0.012 - LR)
        a->kd = values[2];                                                     //(5.0.012 - LR)
        a->e1 = 0.0;                                                           //(5.0.012 - LR)
        a->e2 = 0.0;                                                           //(5.0.012 - LR)
    }                                                                          //(5.0.012 - LR)
    if ( InputState == r_THEN )
    {
        a->next = Rules[r].thenActions;
        Rules[r].thenActions = a;
    }
    else
    {
        a->next = Rules[r].elseActions;
        Rules[r].elseActions = a;
    }
    return 0;
}

//=============================================================================

int  setActionSetting(char* tok[], int nToks, int* curve, int* tseries,        //(5.0.012 - LR)
                      int* attrib, double values[])                            //(5.0.012 - LR)
//
//  Input:   tok = array of string tokens containing action statement
//           nToks = number of string tokens
//  Output:  curve = index of controller curve
//           tseries = index of controller time series
//           attrib = r_PID if PID controller used                             //(5.0.012 - LR)
//           values = values of control settings                               //(5.0.012 - LR)
//           returns an error code
//  Purpose: identifies how control actions settings are determined.
//
{
    int k, m;

    // --- see if control action is determined by a Curve or Time Series
    if (nToks < 6) return error_setInpError(ERR_ITEMS, "");
    k = findmatch(tok[5], SettingTypeWords);
    if ( k >= 0 && nToks < 7 ) return error_setInpError(ERR_ITEMS, "");
    switch (k)
    {

    // --- control determined by a curve - find curve index
    case r_CURVE:
        m = project_findObject(CURVE, tok[6]);
        if ( m < 0 ) return error_setInpError(ERR_NAME, tok[6]);
        *curve = m;
        break;

    // --- control determined by a time series - find time series index
    case r_TIMESERIES:
        m = project_findObject(TSERIES, tok[6]);
        if ( m < 0 ) return error_setInpError(ERR_NAME, tok[6]);
        *tseries = m;
        Tseries[m].refersTo = CONTROL;                                         //(5.0.019 - LR)
        break;

    // --- control determined by PID controller                                //(5.0.012 - LR)
    case r_PID:                                                                //(5.0.012 - LR)
        if (nToks < 9) return error_setInpError(ERR_ITEMS, "");                //(5.0.012 - LR)
        for (m=6; m<=8; m++)                                                   //(5.0.012 - LR)
        {                                                                      //(5.0.012 - LR)
            if ( !getDouble(tok[m], &values[m-6]) )                            //(5.0.012 - LR)
                return error_setInpError(ERR_NUMBER, tok[m]);                  //(5.0.012 - LR)
        }                                                                      //(5.0.012 - LR)
        *attrib = r_PID;                                                       //(5.0.012 - LR)
        break;                                                                 //(5.0.012 - LR)

    // --- direct numerical control is used
    default:
        if ( !getDouble(tok[5], &values[0]) )                                  //(5.0.012 - LR)
            return error_setInpError(ERR_NUMBER, tok[5]);
    }
    return 0;
}

//=============================================================================

void  updateActionValue(struct TAction* a, DateTime currentTime, double dt)    //(5.0.012 - LR)
//
//  Input:   a = an action object
//           currentTime = current simulation date/time (days)                 //(5.0.013 - LR)
//           dt = time step (days)                                             //(5.0.013 - LR)
//  Output:  none
//  Purpose: updates value of actions found from Curves or Time Series.
//
{
    if ( a->curve >= 0 )
    {
        a->value = table_lookup(&Curve[a->curve], ControlValue);
    }
    else if ( a->tseries >= 0 )
    {
        a->value = table_tseriesLookup(&Tseries[a->tseries], currentTime, TRUE);
    }
    else if ( a->attribute == r_PID )
    {
        a->value = getPIDSetting(a, dt);
    }
}

//=============================================================================

////  New function to get PID control setting.  ////                           //(5.0.012 - LR)
////  Function re-written as PID definitions have changed.  ////               //(5.0.013 - LR)

double getPIDSetting(struct TAction* a, double dt)
//
//  Input:   a = an action object
//           dt = current time step (days)
//  Output:  returns a new link setting 
//  Purpose: computes a new setting for a link subject to a PID controller.
//
//  Note:    a->kp = gain coefficient,
//           a->ki = integral time (minutes)
//           a->k2 = derivative time (minutes)
//           a->e1 = error from previous time step
//           a->e2 = error from two time steps ago
{
    double e0, setting;
	double p, i, d, update;
	double tolerance = 0.0001;

	// --- convert time step from days to minutes
	dt *= 1440.0;

    // --- determine relative error in achieving controller set point
    e0 = SetPoint - ControlValue;
    if ( fabs(e0) > TINY )
    {
        if ( SetPoint != 0.0 ) e0 = e0/SetPoint;
        else                   e0 = e0/ControlValue;
    }

	// --- reset previous errors to 0 if controller gets stuck
	if (fabs(e0 - a->e1) < tolerance)
	{
		a->e2 = 0.0;
		a->e1 = 0.0;
	}

    // --- use the recursive form of the PID controller equation to
    //     determine the new setting for the controlled link
	p = (e0 - a->e1);
	if ( a->ki == 0.0 ) i = 0.0;
	else i = e0 * dt / a->ki;
	d = a->kd * (e0 - 2.0*a->e1 + a->e2) / dt;
	update = a->kp * (p + i + d);
	if ( fabs(update) < tolerance ) update = 0.0;
	setting = Link[a->link].targetSetting + update;

	// --- update previous errors
    a->e2 = a->e1;
    a->e1 = e0;

    // --- check that new setting lies within feasible limits
    if ( setting < 0.0 ) setting = 0.0;
    if (Link[a->link].type != PUMP && setting > 1.0 ) setting = 1.0;
    return setting;
}

//=============================================================================

void updateActionList(struct TAction* a)
//
//  Input:   a = an action object
//  Output:  none
//  Purpose: adds a new action to the list of actions to be taken.
//
{
    struct TActionList* listItem;
    struct TAction* a1;
    double priority = Rules[a->rule].priority;

    // --- check if link referred to in action is already listed
    listItem = ActionList;
    while ( listItem )
    {
        a1 = listItem->action;
        if ( !a1 ) break;
        if ( a1->link == a->link )
        {
            // --- replace old action if new action has higher priority
            if ( priority > Rules[a1->rule].priority ) listItem->action = a;
            return;
        }
        listItem = listItem->next;
    }

    // --- action not listed so add it to ActionList
    if ( !listItem )
    {
        listItem = (struct TActionList *) malloc(sizeof(struct TActionList));
        listItem->next = ActionList;
        ActionList = listItem;
    }
    listItem->action = a;
}

//=============================================================================

int executeActionList(DateTime currentTime)
//
//  Input:   currentTime = current date/time of the simulation
//  Output:  returns number of new actions taken
//  Purpose: executes all actions required by fired control rules.
//
{
    struct TActionList* listItem;
    struct TActionList* nextItem;
    struct TAction* a1;
    int count = 0;

    listItem = ActionList;
    while ( listItem )
    {
        a1 = listItem->action;
        if ( !a1 ) break;
        if ( a1->link >= 0 )
        {
            if ( Link[a1->link].targetSetting != a1->value )                   //(5.0.010 - LR)
            {
                Link[a1->link].targetSetting = a1->value;
                if ( RptFlags.controls )
                    report_writeControlAction(currentTime, Link[a1->link].ID,
                                              a1->value, Rules[a1->rule].ID);
                count++;
            }
        }
        nextItem = listItem->next;
        listItem = nextItem;
    }
    return count;
}

//=============================================================================

int evaluatePremise(struct TPremise* p, DateTime theDate, DateTime theTime,
                    DateTime elapsedTime, double tStep)
//
//  Input:   p = a control rule premise condition
//           theDate = the current simulation date
//           theTime = the current simulation time of day
//           elpasedTime = decimal days since the start of the simulation
//           tStep = current time step (days)                                  //(5.0.013 - LR)
//  Output:  returns TRUE if the condition is true or FALSE otherwise
//  Purpose: evaluates the truth of a control rule premise condition.
//
{
    int i = p->node;
    int j = p->link;
    double head;

    switch ( p->attribute )
    {
      case r_TIME:
        return checkTimeValue(p, elapsedTime, elapsedTime + tStep);

      case r_DATE:
        return checkValue(p, theDate);

      case r_CLOCKTIME:
        return checkTimeValue(p, theTime, theTime + tStep);

      case r_DAY:                                                              //(5.0.014 - LR)
        return checkValue(p, datetime_dayOfWeek(theDate));                     //(5.0.014 - LR)

      case r_MONTH:                                                            //(5.0.014 - LR)
        return checkValue(p, datetime_monthOfYear(theDate));                   //(5.0.014 - LR)

      case r_STATUS:
        if ( j < 0 || Link[j].type != PUMP ) return FALSE;
        else return checkValue(p, Link[j].setting);
        
      case r_SETTING:
        if ( j < 0 || (Link[j].type != ORIFICE && Link[j].type != WEIR) )
            return FALSE;
        else return checkValue(p, Link[j].setting);

      case r_FLOW:
        if ( j < 0 ) return FALSE;
        else return checkValue(p, Link[j].direction*Link[j].newFlow*UCF(FLOW));//(5.0.019 - LR)

      case r_DEPTH:
        if ( j >= 0 ) return checkValue(p, Link[j].newDepth*UCF(LENGTH));
        else if ( i >= 0 )
            return checkValue(p, Node[i].newDepth*UCF(LENGTH));
        else return FALSE;

      case r_HEAD:
        if ( i < 0 ) return FALSE;
        head = (Node[i].newDepth + Node[i].invertElev) * UCF(LENGTH);
        return checkValue(p, head);

      case r_INFLOW:
        if ( i < 0 ) return FALSE;
        else return checkValue(p, Node[i].newLatFlow*UCF(FLOW));

      default: return FALSE;
    }
}

//=============================================================================

int checkTimeValue(struct TPremise* p, double time1, double time2)
//
//  Input:   p = control rule premise condition
//           time1 = time of day or elapsed time at start of current time step
//           time2 = time of day or elapsed time at end of current time step
//  Output:  returns TRUE if time condition is satisfied
//  Purpose: evaluates the truth of a condition involving time.
//
{
    if ( p->operand == EQ )
    {
        if ( p->value >= time1 && p->value < time2 ) return TRUE;
        return FALSE;
    }
    else if ( p->operand == NE )
    {
        if ( p->value < time1 || p->value >= time2 ) return TRUE;
        return FALSE;
    }
    else return checkValue(p, time1);
}

//=============================================================================

int checkValue(struct TPremise* p, double x)
//
//  Input:   p = control rule premise condition
//           x = value being compared to value in the condition
//  Output:  returns TRUE if condition is satisfied
//  Purpose: evaluates the truth of a condition involving a numerical comparison.
//
{
    SetPoint = p->value;                                                       //(5.0.012 - LR)
    ControlValue = x;
    switch (p->operand)
    {
      case EQ: if ( x == p->value ) return TRUE; break;
      case NE: if ( x != p->value ) return TRUE; break;
      case LT: if ( x <  p->value ) return TRUE; break;
      case LE: if ( x <= p->value ) return TRUE; break;
      case GT: if ( x >  p->value ) return TRUE; break;
      case GE: if ( x >= p->value ) return TRUE; break;
    }
    return FALSE;
}

//=============================================================================

void clearActionList(void)
//
//  Input:   none
//  Output:  none
//  Purpose: clears the list of actions to be executed.
//
{
    struct TActionList* listItem;
    listItem = ActionList;
    while ( listItem )
    {
        listItem->action = NULL;
        listItem = listItem->next;
    }
}

//=============================================================================

void  deleteActionList(void)
//
//  Input:   none
//  Output:  none
//  Purpose: frees the memory used to hold the list of actions to be executed.
//
{
    struct TActionList* listItem;
    struct TActionList* nextItem;
    listItem = ActionList;
    while ( listItem )
    {
        nextItem = listItem->next;
        free(listItem);
        listItem = nextItem;
    }
    ActionList = NULL;
}

//=============================================================================

void  deleteRules(void)
//
//  Input:   none
//  Output:  none
//  Purpose: frees the memory used for all of the control rules.
//
{
   struct TPremise* p;
   struct TPremise* pnext;
   struct TAction*  a;
   struct TAction*  anext;
   int r;
   for (r=0; r<RuleCount; r++)
   {
      p = Rules[r].firstPremise;
      while ( p )
      {
         pnext = p->next;
         free(p);
         p = pnext;
      }
      a = Rules[r].thenActions;
      while (a )
      {
         anext = a->next;
         free(a);
         a = anext;
      }
      a = Rules[r].elseActions;
      while (a )
      {
         anext = a->next;
         free(a);
         a = anext;
      }
   }
   FREE(Rules);
   RuleCount = 0;
}

//=============================================================================

int  findExactMatch(char *s, char *keyword[])
//
//  Input:   s = character string
//           keyword = array of keyword strings
//  Output:  returns index of keyword which matches s or -1 if no match found  
//  Purpose: finds exact match between string and array of keyword strings.
//
{
   int i = 0;
   while (keyword[i] != NULL)
   {
      if ( strcomp(s, keyword[i]) ) return(i);
      i++;
   }
   return(-1);
}

//=============================================================================
