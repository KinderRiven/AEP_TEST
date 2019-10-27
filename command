Commands:
Display the CLI help.
    help [-help|-h] [-output|-o (text|nvmxml)]

Display the CLI version.
    version [-help|-h] [-output|-o (text|nvmxml)] 
    
Update the firmware on one or more DIMMs
    load [-help|-h] [-examine|-x (Verify only)] [-force|-f (Suppress confirmation)] -source (
path) [-output|-o (text|nvmxml)] [-recover (FlashSPI)] -dimm [(DimmIDs)] Set properties of one or more DIMMs.
    set [-help|-h] [-force|-f] [-source (path)] [-output|-o (text|nvmxml)] -dimm [(DimmIDs)] 
[Clear=(1)] [Temperature=(value)] [Poison=(value)] [PoisonType=(value)] [PackageSparing=(1)] [PercentageRemaining=(0|%)] [FatalMediaError=(1)] [DirtyShutdown=(1)] [LockState=(Disabled|Unlocked|Frozen)] [Passphrase=(string)] [NewPassphrase=(string)] [ConfirmPassphrase=(string)] Erase persistent data on one or more DIMMs.
    delete [-help|-h] [-force|-f] [-source (path)] [-output|-o (text|nvmxml)] -dimm [(DimmIDs
)] [Passphrase=(string)] Show information about one or more Regions.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] -region [(RegionIDs)] [-socket (SocketIDs)] Provision capacity on one or more DIMMs into regions
    create [-help|-h] [-force|-f] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)] [-output|-o (text|nvmx
ml)] [-dimm [(DimmIDs)]] -goal [-socket (SocketIDs)] [MemoryMode=(0|%)] [PersistentMemoryType=(AppDirect|AppDirectNotInterleaved)] [Reserved=(0|%)] [NamespaceLabelVersion=(1.1|1.2)] Show region configuration goal stored on one or more DIMMs
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] [-dimm [(DimmIDs)]] -goal [-socket (SocketIDs)] Delete the region configuration goal from one or more DIMMs
    delete [-help|-h] [-output|-o (text|nvmxml)] [-dimm [(DimmIDs)]] -goal [-socket (SocketID
s)] Load stored configuration goal for specific DIMMs
    load [-help|-h] [-force|-f] -source (path) [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)] [-output|
-o (text|nvmxml)] [-dimm [(DimmIDs)]] -goal [-socket (SocketIDs)] Store the region configuration goal from one or more DIMMs to a file
    dump [-help|-h] -destination (file) [-output|-o (text|nvmxml)] -system -config 
Modify the alarm threshold(s) for one or more DIMMs.
    set [-help|-h] [-force|-f] [-output|-o (text|nvmxml)] -sensor (MediaTemperature|Controlle
rTemperature|PercentageRemaining) [-dimm [(DimmIDs)]] [NonCriticalThreshold=(value)] [EnabledState=(0|1)] Clear the namespace LSA partition on one or more DIMMs
    delete [-help|-h] [-force|-f] [-output|-o (text|nvmxml)] -dimm [(DimmIDs)] -pcd [(Config)
] Show error log for given DIMM
    show [-help|-h] [-output|-o (text|nvmxml)] -error (Thermal|Media) -dimm [(DimmIDs)] [Sequ
enceNumber=(<1, 65535>)] [Level=(Low|High)] [Count=(<0, 255>)] Dump firmware debug log
    dump [-help|-h] -destination (file_prefix (prefix for output files)) [-output|-o (text|nv
mxml)] [-dict (file)] -debug -dimm [(DimmIDs)] Show information about one or more DIMMs.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] -dimm [(DimmIDs)] [-socket (SocketIDs)] Show basic information about the physical     processors in the host server.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] -socket [(SocketIDs)] Show health statistics 
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-output|-o (text|nvmxml)] -sensor [
(Health|MediaTemperature|ControllerTemperature|PercentageRemaining|LatchedDirtyShutdownCount|PowerOnTime|UpTime|PowerCycles|FwErrorCount|UnlatchedDirtyShutdownCount)] [-dimm [(DimmIDs)]] Run a diagnostic test on one or more DIMMs
    start [-help|-h] [-output|-o (text|nvmxml)] -diagnostic [(Quick|Config|Security|FW)] [-di
mm [(DimmIDs)]] Show the topology of the DCPMMs installed in the host server
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] -topology [-dimm [(DimmIDs)]] [-socket (SocketIDs)] Show information about total DIMM resource allocation.
    show [-help|-h] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)] [-output|-o (text|nvmxml)] -memoryre
sources Show information about BIOS memory management capabilities.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-units|-u (B|MB|MiB|GB|GiB|TB|TiB)]
 [-output|-o (text|nvmxml)] -system -capabilities Show information about firmware on one or more DIMMs.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-output|-o (text|nvmxml)] [-dimm [(
DimmIDs)]] -firmware Show the ACPI tables related to the DIMMs in the system.
    show [-help|-h] [-output|-o (text|nvmxml)] -system [(NFIT|PCAT|PMTT)] 
Show pool configuration goal stored on one or more DIMMs
    show [-help|-h] [-output|-o (text|nvmxml)] -dimm [(DimmIDs)] -pcd [(Config|LSA)] 
Show user preferences and their current values
    show [-help|-h] [-output|-o (text|nvmxml)] -preferences 
Set user preferences
    set [-help|-h] [-output|-o (text|nvmxml)] -preferences [CLI_DEFAULT_DIMM_ID=(HANDLE|UID)]
 [CLI_DEFAULT_SIZE=(AUTO|AUTO_10|B|MB|MiB|GB|GiB|TB|TiB)] [APPDIRECT_SETTINGS=(RECOMMENDED|(IMCSize)_(ChannelSize))] [APPDIRECT_GRANULARITY=(RECOMMENDED|1)] [PERFORMANCE_MONITOR_ENABLED=(0|1)] [PERFORMANCE_MONITOR_INTERVAL_MINUTES=(minutes)] [EVENT_MONITOR_ENABLED=(0|1)] [EVENT_MONITOR_INTERVAL_MINUTES=(minutes)] [EVENT_LOG_MAX=(num events)] [DBG_LOG_MAX=(num log entries)] [DBG_LOG_LEVEL=(log level)] Show Command Access Policy Restrictions for DIMM(s).
    show [-help|-h] [-output|-o (text|nvmxml)] [-dimm [(DimmIDs)]] -cap 
Show basic information about the host server.
    show [-help|-h] [-all|-a] [-display|-d (Attributes)] [-output|-o (text|nvmxml)] -system -
host Show event stored on one in the system log
    show [-help|-h] [-output|-o (text|nvmxml)] -event [-dimm [(DimmIDs)]] [Severity=(Info|War
ning|Error)] [Category=(Diag|FW|Config|PM|Quick|Security|Health|Mgmt)] [ActionRequired=(1|0)] [Count=(<1..2147483647>)] Set event's action required flag on/off
    set [-help|-h] [-output|-o (text|nvmxml)] -event (EventID) ActionRequired=(0) 
Capture a snapshot of the system state for support purposes
    dump [-help|-h] -destination (file_prefix (prefix for output files)) [-output|-o (text|nv
mxml)] [-dict (file)] -support Show performance statistics per DIMM
    show [-help|-h] [-output|-o (text|nvmxml)] -dimm [(DimmIDs)] -performance [(MediaReads|Me
diaWrites|ReadRequests|WriteRequests|TotalMediaReads|TotalMediaWrites|TotalReadRequests|TotalWriteRequests)]
