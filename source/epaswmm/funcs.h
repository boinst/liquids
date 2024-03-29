//-----------------------------------------------------------------------------
//   funcs.h
//
//   Project:  EPA SWMM5
//   Version:  5.0
//   Date:     6/19/07   (Build 5.0.010)
//             2/4/08    (Build 5.0.012)
//             1/21/09   (Build 5.0.014)
//             4/10/09   (Build 5.0.015)
//             11/18/09  (Build 5.0.018)
//             07/30/10  (Build 5.0.019)
//   Author:   L. Rossman
//
//   Global interfacing functions.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//   Project Manager Methods
//-----------------------------------------------------------------------------
void     project_open(char *f1, char *f2, char *f3);
void     project_close(void);
void     project_readInput(void);
int      project_readOption(char* s1, char* s2);
void     project_validate(void);
int      project_init(void);
int      project_addObject(int type, char* id, int n);
int      project_findObject(int type, char* id);
char*    project_findID(int type, char* id);
double** project_createMatrix(int nrows, int ncols);
void     project_freeMatrix(double** m);

//-----------------------------------------------------------------------------
//   Input Reader Methods
//-----------------------------------------------------------------------------
int     input_countObjects(void);
int     input_readData(void);

//-----------------------------------------------------------------------------
//   Report Writer Methods
//-----------------------------------------------------------------------------
int     report_readOptions(char* tok[], int ntoks);
void    report_writeLine(char* line);
void    report_writeSysTime(void);
void    report_writeLogo(void);
void    report_writeTitle(void);
void    report_writeRainStats(int gage, TRainStats* rainStats);
void    report_writeRdiiStats(double totalRain, double totalRdii);
void    report_writeControlActionsHeading(void);
void    report_writeControlAction(DateTime aDate, char* linkID, double value,
        char* ruleID);
void    report_writeRunoffError(TRunoffTotals* totals, double area);
void    report_writeLoadingError(TLoadingTotals* totals);
void    report_writeGwaterError(TGwaterTotals* totals, double area);
void    report_writeFlowError(TRoutingTotals* totals);
void    report_writeQualError(TRoutingTotals* totals);
void    report_writeMaxStats(TMaxStats massBalErrs[], TMaxStats CourantCrit[],
        int nMaxStats);
void    report_writeMaxFlowTurns(TMaxStats flowTurns[], int nMaxStats);        //(5.0.010 - LR)
void    report_writeSysStats(TSysStats* sysStats);
void    report_writeReport(void);
void    report_writeErrorMsg(int code, char* msg);
void    report_writeErrorCode(void);
void    report_writeInputErrorMsg(int k, int sect, char* line, long lineCount);

void    inputrpt_writeInput(void);                                             //(5.0.012 - LR)
void    statsrpt_writeReport(void);                                            //(5.0.012 - LR)
void    report_writeWarningMsg(char* msg, char* id);                           //(5.0.014 - LR)
void    report_writeTseriesErrorMsg(TTable *tseries);                          //(5.0.014 - LR)

//-----------------------------------------------------------------------------
//   Temperature/Evaportation Methods
//-----------------------------------------------------------------------------
int      climate_readParams(char* tok[], int ntoks);
int      climate_readEvapParams(char* tok[], int ntoks);
void     climate_validate(void);
void     climate_openFile(void);
void     climate_initState(void);
void     climate_setState(DateTime aDate);
DateTime climate_getNextEvap(DateTime aDate); 

//-----------------------------------------------------------------------------
//   Rainfall Processing Methods
//-----------------------------------------------------------------------------
void    rain_open(void);
void    rain_close(void);

//-----------------------------------------------------------------------------
//   Snowmelt Processing Methods
//-----------------------------------------------------------------------------
int     snow_readMeltParams(char* tok[], int ntoks);
int     snow_createSnowpack(int subcacth, int snowIndex);
void    snow_initSnowpack(int subcatch);
void    snow_initSnowmelt(int snowIndex);
void    snow_validateSnowmelt(int snowIndex);                                  //(5.0.012 - LR)
void    snow_setMeltCoeffs(int snowIndex, double season);
void    snow_plowSnow(int subcatch, double tStep);
double  snow_getSnowMelt(int subcatch, double rainfall, double snowfall,
        double tStep, double netPrecip[]);
double  snow_getSnowCover(int subcatch);

//-----------------------------------------------------------------------------
//   Runoff Analyzer Methods
//-----------------------------------------------------------------------------
int     runoff_open(void);
void    runoff_execute(void);
void    runoff_close(void);

//-----------------------------------------------------------------------------
//   Conveyance System Routing Methods
//-----------------------------------------------------------------------------
int     routing_open(int routingModel);
double  routing_getRoutingStep(int routingModel, double fixedStep);
void    routing_execute(int routingModel, double routingStep);
void    routing_close(int routingModel);

//-----------------------------------------------------------------------------
//   Output Filer Methods
//-----------------------------------------------------------------------------
int     output_open(void);
void    output_end(void);
void    output_close(void);
void    output_checkFileSize(void);                                            //(5.0.015 - LR)
void    output_saveResults(double reportTime);
void    output_readDateTime(int period, DateTime *aDate);                      //(5.0.014 - LR)
void    output_readSubcatchResults(int period, int area);                      //(5.0.014 - LR)
void    output_readNodeResults(int period, int node);                          //(5.0.014 - LR)
void    output_readLinkResults(int period, int link);                          //(5.0.014 - LR)

//-----------------------------------------------------------------------------
//   Infiltration Methods
//-----------------------------------------------------------------------------
//    These methods now appear in infil.h.  ////                               //(5.0.019 - LR)

//-----------------------------------------------------------------------------
//   Groundwater Methods
//-----------------------------------------------------------------------------
int     gwater_readAquiferParams(int aquifer, char* tok[], int ntoks);
int     gwater_readGroundwaterParams(char* tok[], int ntoks);
void    gwater_validateAquifer(int aquifer);
void    gwater_initState(int subcatch);
void    gwater_getGroundwater(int subcatch, double evap, double infil,         //(5.0.019 - LR)
        double tStep);
double  gwater_getVolume(int subcatch);

//-----------------------------------------------------------------------------
//   RDII Methods
//-----------------------------------------------------------------------------
int     rdii_readRdiiInflow(char* tok[], int ntoks);
void    rdii_deleteRdiiInflow(int node);
void    rdii_initUnitHyd(int unitHyd);
int     rdii_readUnitHydParams(char* tok[], int ntoks);
void    rdii_openRdii(void);
void    rdii_closeRdii(void);
int     rdii_getNumRdiiFlows(DateTime aDate);
void    rdii_getRdiiFlow(int index, int* node, double* q);

//-----------------------------------------------------------------------------
//   Landuse Methods
//-----------------------------------------------------------------------------
int     landuse_readParams(int landuse, char* tok[], int ntoks);
int     landuse_readPollutParams(int pollut, char* tok[], int ntoks);
int     landuse_readBuildupParams(char* tok[], int ntoks);
int     landuse_readWashoffParams(char* tok[], int ntoks);
double  landuse_getBuildup(int landuse, int pollut, double area, double curb,
        double buildup, double tStep);
void    landuse_getWashoff(int landuse, double area, TLandFactor landFactor[],
        double runoff, double tStep, double washoffLoad[]);

//-----------------------------------------------------------------------------
//   Flow/Quality Routing Methods
//-----------------------------------------------------------------------------
void    flowrout_init(int routingModel);
void    flowrout_close(int routingModel);
double  flowrout_getRoutingStep(int routingModel, double fixedStep);
int     flowrout_execute(int links[], int routingModel, double tStep);

void    qualrout_execute(double tStep);
double  qualrout_getCstrQual(double c, double v, double wIn, double qIn,       //(5.0.014 - LR)
        double kDecay, double tStep);                                          //(5.0.014 - LR)

void    toposort_sortLinks(int links[]);

int     kinwave_execute(int link, double* qin, double* qout, double tStep);

void    dynwave_init(void);
void    dynwave_close(void);
double  dynwave_getRoutingStep(double fixedStep);
int     dynwave_execute(int links[], double tStep);

//-----------------------------------------------------------------------------
//   Treatment Methods
//-----------------------------------------------------------------------------
int     treatmnt_open(void);
void    treatmnt_close(void);
int     treatmnt_readExpression(char* tok[], int ntoks);
void    treatmnt_delete(int node);
void    treatmnt_treat(int node, double q, double v, double tStep);
void    treatmnt_setInflow(double qIn, double wIn[]);

//-----------------------------------------------------------------------------
//   Mass Balance Methods
//-----------------------------------------------------------------------------
int     massbal_open(void);
void    massbal_close(void);
void    massbal_report(void);
void    massbal_updateRunoffTotals(double vRainfall, double vEvap, double vInfil,
        double vRunoff);
void    massbal_updateLoadingTotals(int type, int pollut, double w);
void    massbal_updateGwaterTotals(double vInfil, double vUpperEvap,
        double vLowerEvap, double vLowerPerc, double vGwater);
void    massbal_updateRoutingTotals(double tStep);
void    massbal_initTimeStepTotals(void);
void    massbal_addInflowFlow(int type, double q);
void    massbal_addInflowQual(int type, int pollut, double w);
void    massbal_addOutflowFlow(double q, int isFlooded);
void    massbal_addOutflowQual(int pollut, double mass, int isFlooded);
void    massbal_addNodeLosses(double losses);                                  //(5.0.015 - LR)
void    massbal_addReactedMass(int pollut, double mass);
double  massbal_getStepFlowError(void);                                        //(5.0.012 - LR)

//-----------------------------------------------------------------------------
//   Simulation Statistics Methods
//-----------------------------------------------------------------------------
int     stats_open(void);
void    stats_close(void);
void    stats_report(void);
void    stats_updateCriticalTimeCount(int node, int link);
void    stats_updateFlowStats(double tStep, DateTime aDate, int stepCount,
        int steadyState);
void    stats_updateSubcatchStats(int subcatch, double rainVol, double runonVol,
        double evapVol, double infilVol, double runoffVol, double runoff);
void    stats_updateMaxRunoff(void);

//-----------------------------------------------------------------------------
//   Raingage Methods
//-----------------------------------------------------------------------------
int      gage_readParams(int gage, char* tok[], int ntoks);
void     gage_validate(int gage);                                              //(5.0.018 - LR)
void     gage_initState(int gage);
void     gage_setState(int gage, DateTime aDate);
double   gage_getPrecip(int gage, double *rainfall, double *snowfall);
void     gage_setReportRainfall(int gage, DateTime aDate);
DateTime gage_getNextRainDate(int gage, DateTime aDate);

//-----------------------------------------------------------------------------
//   Subcatchment Methods
//-----------------------------------------------------------------------------
int     subcatch_readParams(int subcatch, char* tok[], int ntoks);
int     subcatch_readSubareaParams(char* tok[], int ntoks);
int     subcatch_readLanduseParams(char* tok[], int ntoks);
int     subcatch_readInitBuildup(char* tok[], int ntoks);
void    subcatch_validate(int subcatch);
void    subcatch_initState(int subcatch);
void    subcatch_setOldState(int subcatch);
double  subcatch_getFracPerv(int subcatch);                                    //(5.0.019 - LR)
double  subcatch_getStorage(int subcatch);                                     //(5.0.019 - LR)
void    subcatch_getRunon(int subcatch);
double  subcatch_getRunoff(int subcatch, double tStep);
double  subcatch_getDepth(int subcatch);
void    subcatch_getWashoff(int subcatch, double runoff, double tStep);
void    subcatch_getBuildup(int subcatch, double tStep);
void    subcatch_sweepBuildup(int subcatch, DateTime aDate);
double  subcatch_getWtdOutflow(int subcatch, double wt);
double  subcatch_getWtdWashoff(int subcatch, int pollut, double wt);
void    subcatch_getResults(int subcatch, double wt, float x[]);

//-----------------------------------------------------------------------------
//   Conveyance System Node Methods
//-----------------------------------------------------------------------------
int     node_readParams(int node, int type, int subIndex, char* tok[], int ntoks);
void    node_validate(int node);
void    node_initState(int node);
void    node_setOldHydState(int node);
void    node_setOldQualState(int node);
void    node_initInflow(int node, double tStep);
void    node_setOutletDepth(int node, double yNorm, double yCrit, double z);
void    node_setDividerCutoff(int node, int link);
double  node_getSurfArea(int node, double depth);
double  node_getDepth(int node, double volume);
double  node_getVolume(int node, double depth);
double  node_getPondedDepth(int node, double volume);
double  node_getPondedArea(int node, double depth);
double  node_getOutflow(int node, int link);
double  node_getLosses(int node, double tStep);                                //(5.0.019 - LR)
double  node_getMaxOutflow(int node, double q, double tStep);
double  node_getSystemOutflow(int node, int *isFlooded);
void    node_getResults(int node, double wt, float x[]);

//-----------------------------------------------------------------------------
//   Conveyance System Inflow Methods
//-----------------------------------------------------------------------------
int     inflow_readExtInflow(char* tok[], int ntoks);
int     inflow_readDwfInflow(char* tok[], int ntoks);
int     inflow_readDwfPattern(char* tok[], int ntoks);
void    inflow_initDwfPattern(int pattern);
double  inflow_getExtInflow(TExtInflow* inflow, DateTime aDate);
double  inflow_getDwfInflow(TDwfInflow* inflow, int m, int d, int h);
double  inflow_getPatternFactor(int p, int month, int day, int hour);          //(5.0.019 - LR)
void    inflow_deleteExtInflows(int node);
void    inflow_deleteDwfInflows(int node);

//-----------------------------------------------------------------------------
//   Routing Interface File Methods
//-----------------------------------------------------------------------------
int     iface_readFileParams(char* tok[], int ntoks);
void    iface_openRoutingFiles(void);
void    iface_closeRoutingFiles(void);
int     iface_getNumIfaceNodes(DateTime aDate);
int     iface_getIfaceNode(int index);
double  iface_getIfaceFlow(int index);
double  iface_getIfaceQual(int index, int pollut);
void    iface_saveOutletResults(DateTime reportDate, FILE* file);

//-----------------------------------------------------------------------------
//   Conveyance System Link Methods
//-----------------------------------------------------------------------------
int     link_readParams(int link, int type, int subIndex, char* tok[], int ntoks);
int     link_readXsectParams(char* tok[], int ntoks);
int     link_readLossParams(char* tok[], int ntoks);
void    link_validate(int link);
void    link_initState(int link);
void    link_setOldHydState(int link);
void    link_setOldQualState(int link);
void    link_setTargetSetting(int j);                                          //(5.0.010 - LR)
void    link_setSetting(int j, double tstep);                                  //(5.0.010 - LR)
int     link_setFlapGate(int link, int n1, int n2, double q);                  //(5.0.014 - LR)
double  link_getInflow(int link);
void    link_setOutfallDepth(int link);
double  link_getLength(int link);                                              //(5.0.015 - LR)
double  link_getYcrit(int link, double q);
double  link_getYnorm(int link, double q);
double  link_getVelocity(int link, double q, double y);
double  link_getFroude(int link, double v, double y);
double  link_getPower(int link);                                               //(5.0.012 - LR)
void    link_getResults(int link, double wt, float x[]);

//-----------------------------------------------------------------------------
//   Link Cross-Section Methods
//-----------------------------------------------------------------------------
int     xsect_isOpen(int type);
int     xsect_setParams(TXsect *xsect, int type, double p[], double ucf);
void    xsect_setIrregXsectParams(TXsect *xsect);
void    xsect_setCustomXsectParams(TXsect *xsect);                             //(5.0.010 - LR)
double  xsect_getAmax(TXsect* xsect);
double  xsect_getSofA(TXsect* xsect, double area);
double  xsect_getYofA(TXsect* xsect, double area);
double  xsect_getRofA(TXsect* xsect, double area);
double  xsect_getAofS(TXsect* xsect, double sFactor);
double  xsect_getdSdA(TXsect* xsect, double area);
double  xsect_getAofY(TXsect* xsect, double y);
double  xsect_getRofY(TXsect* xsect, double y);
double  xsect_getWofY(TXsect* xsect, double y);
double  xsect_getYcrit(TXsect* xsect, double q);

//-----------------------------------------------------------------------------//(5.0.014 - LR)
//   Culvert Methods                                                           //(5.0.014 - LR)
//-----------------------------------------------------------------------------//(5.0.014 - LR)
double  culvert_getInflow(int link, double q, double h);                       //(5.0.014 - LR)

//-----------------------------------------------------------------------------//(5.0.010 - LR)
//   Force Main Methods                                                        //(5.0.010 - LR)
//-----------------------------------------------------------------------------//(5.0.010 - LR)
double  forcemain_getEquivN(int j, int k);                                     //(5.0.010 - LR)
double  forcemain_getRoughFactor(int j, double lengthFactor);                  //(5.0.010 - LR)
double  forcemain_getFricSlope(int j, double v, double hrad);                  //(5.0.010 - LR)

//-----------------------------------------------------------------------------
//   Cross-Section Transect Methods
//-----------------------------------------------------------------------------
int     transect_create(int n);
void    transect_delete(void);
int     transect_readParams(int* count, char* tok[], int ntoks);
void    transect_validate(int j);

//-----------------------------------------------------------------------------//(5.0.010 - LR)
//   Custom Shape Cross-Section Methods                                        //(5.0.010 - LR)
//-----------------------------------------------------------------------------//(5.0.010 - LR)
int     shape_validate(TShape *shape, TTable *curve);                          //(5.0.010 - LR)

//-----------------------------------------------------------------------------
//   Control Rule Methods
//-----------------------------------------------------------------------------
int     controls_create(int n);
void    controls_delete(void);
int     controls_addRuleClause(int rule, int keyword, char* Tok[], int nTokens);
int     controls_evaluate(DateTime currentTime, DateTime elapsedTime, 
        double tStep);

//-----------------------------------------------------------------------------
//   Table & Time Series Methods
//-----------------------------------------------------------------------------
int     table_readCurve(char* tok[], int ntoks);
int     table_readTimeseries(char* tok[], int ntoks);
int     table_addEntry(TTable* table, double x, double y);
void    table_deleteEntries(TTable* table);
void    table_init(TTable* table);
int     table_validate(TTable* table);
double  table_interpolate(double x, double x1, double y1, double x2, double y2);
double  table_lookup(TTable* table, double x);
double  table_intervalLookup(TTable* table, double x);
double  table_inverseLookup(TTable* table, double y);
int     table_getFirstEntry(TTable* table, double* x, double* y);
//int     table_getLastEntry(TTable *table, double *x, double *y);             //(5.0.014 - LR)
int     table_getNextEntry(TTable* table, double* x, double* y);
void    table_tseriesInit(TTable *table);
double  table_tseriesLookup(TTable* table, double t, char extend);
double  table_getArea(TTable* table, double x);
double  table_getInverseArea(TTable* table, double a);
double  table_lookupEx(TTable* table, double x);

//-----------------------------------------------------------------------------
//   Utility Methods
//-----------------------------------------------------------------------------
double   UCF(int quantity);                   // units conversion factor
int      getInt(char *s, int *y);             // get integer from string
int      getFloat(char *s, float *y);         // get float from string
int      getDouble(char *s, double *y);       // get double from string
char*    getTmpName(char *s);                 // get temporary file name
int      findmatch(char *s, char *keyword[]); // search for matching keyword
int      match(char *str, char *substr);      // true if substr matches part of str
int      strcomp(char *s1, char *s2);         // case insensitive string compare
char*    sstrncpy(char *dest, const char *src,
         size_t maxlen);                      // safe string copy
void     writecon(char *s);                   // writes string to console
DateTime getDateTime(double elapsedMsec);     // convert elapsed time to date
void     getElapsedTime(DateTime aDate,       // convert elapsed date
         int* days, int* hrs, int* mins);
