-- Read server\readme_tables.txt for comments and descriptions of these tables and columns
-- InnoDB must be used because PRPNet needs a database engine that supports transactions
-- latin1_bin is used because column values are case sensitive

drop table if exists CandidateTestResult;
drop table if exists GeneferROE;
drop table if exists CandidateGFNDivisor;
drop table if exists CandidateGroupStats;
drop table if exists CandidateTest;
drop table if exists Candidate;
drop table if exists UserWWWWs;
drop table if exists UserStats;
drop table if exists UserPrimes;
drop table if exists UserTeamStats;
drop table if exists TeamStats;
drop table if exists WWWWGroupStats;
drop table if exists WWWWRangeTestResult;
drop table if exists WWWWRangeTest;
drop table if exists WWWWRange;

create table UserStats (
   UserID                  varchar(200)   collate latin1_bin,
   TestsPerformed          int            default 0 not null,
   PRPsFound               int            default 0 not null,
   PrimesFound             int            default 0 not null,
   GFNDivisorsFound        int            default 0 not null,
   WWWWPrimes              int            default 0 not null,
   NearWWWWPrimes          int            default 0 not null,
   TotalScore              double,
   primary key (UserID)
) ENGINE=InnoDB;

create table UserWWWWs (
   UserID                  varchar(200)      collate latin1_bin,
   Prime                   bigint            not null,
   Remainder               int,
   Quotient                int,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   DateReported            bigint            not null,
   ShowOnWebPage           int,
   primary key (UserID, Prime)
) ENGINE=InnoDB;

create table UserPrimes (
   UserID                  varchar(200)      collate latin1_bin,
   CandidateName           varchar(50)       not null,
   TestedNumber            varchar(50)       not null,
   TestResult              int               default 1,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   DecimalLength           double,
   DateReported            bigint            not null,
   ShowOnWebPage           int,
   primary key (UserID, TestedNumber)
) ENGINE=InnoDB;

create table UserTeamStats (
   UserID                  varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   TestsPerformed          int               default 0 not null,
   PRPsFound               int               default 0 not null,
   PrimesFound             int               default 0 not null,
   GFNDivisorsFound        int               default 0 not null,
   WWWWPrimes              int               default 0 not null,
   NearWWWWPrimes          int               default 0 not null,
   TotalScore              double,
   primary key (UserID, TeamID)
) ENGINE=InnoDB;

create table TeamStats (
   TeamID                  varchar(200)      collate latin1_bin,
   TestsPerformed          int               default 0 not null,
   PRPsFound               int               default 0 not null,
   PrimesFound             int               default 0 not null,
   GFNDivisorsFound        int               default 0 not null,
   WWWWPrimes              int               default 0 not null,
   NearWWWWPrimes          int               default 0 not null,
   TotalScore              double,
   primary key (TeamID)
) ENGINE=InnoDB;

create table Candidate (
   CandidateName           varchar(50)       not null,
   DecimalLength           double,
   CompletedTests          int               default 0 not null,
   HasPendingTest          int               default 0 not null,
   DoubleChecked           int               default 0 not null,
   k                       bigint,
   b                       int,
   n                       int,
   c                       int,
   MainTestResult          int,
   LastUpdateTime          bigint            not null,
   primary key (CandidateName),
   index ix_bkcn (b, k, c, n),
   index ix_bnck (b, n, c, k),
   index ix_completed (CompletedTests),
   index ix_length (DecimalLength)
) ENGINE=InnoDB;

create table CandidateTest (
   CandidateName           varchar(50)       not null,
   TestID                  bigint            not null,
   IsCompleted             int               default 0 not null,
   EmailID                 varchar(200)      collate latin1_bin,
   UserID                  varchar(200)      collate latin1_bin,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   EmailSent               int,
   primary key (CandidateName, TestID),
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict,
   index ix_userid (UserID),
   index ix_teamid (TeamID)
) ENGINE=InnoDB;

create table CandidateTestResult (
   CandidateName           varchar(50)       not null,
   TestID                  bigint            not null,
   TestIndex               int               not null,
   WhichTest               varchar(20)       collate latin1_bin,
   TestedNumber            varchar(50)       not null,
   TestResult              int               default 0 not null,
   Residue                 varchar(20)       collate latin1_bin,
   PRPingProgram           varchar(50)       collate latin1_bin,
   ProvingProgram          varchar(50)       collate latin1_bin,
   PRPingProgramVersion    varchar(50)       collate latin1_bin,
   ProvingProgramVersion   varchar(50)       collate latin1_bin,
   SecondsForTest          double            default 0,
   CheckedGFNDivisibility  int               default 0,
   primary key (CandidateName, TestID, TestIndex),
   foreign key (CandidateName, TestID) references CandidateTest (CandidateName, TestID) on delete restrict,
   index ix_residue (CandidateName, WhichTest, Residue)
) ENGINE=InnoDB;

create table CandidateGFNDivisor (
   CandidateName           varchar(50)       not null,
   TestedNumber            varchar(50)       not null,
   GFN                     varchar(50)       not null,
   EmailID                 varchar(200)      collate latin1_bin,
   UserID                  varchar(200)      collate latin1_bin,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   primary key (CandidateName, GFN),
   index ix_userid (UserID),
   foreign key (CandidateName) references CandidateTest (CandidateName) on delete restrict
) ENGINE=InnoDB;

create table GeneferROE (
   CandidateName           varchar(50),
   GeneferVersion          varchar(20) collate latin1_bin,
   primary key (CandidateName, GeneferVersion),
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict
) ENGINE=InnoDB;

create table CandidateGroupStats (
   k                       bigint            default 0,
   b                       int               default 0,
   n                       int               default 0,
   c                       int               default 0,
   CountInGroup            int               default 0,
   CountInProgress         int               default 0,
   CountTested             int               default 0,
   CountDoubleChecked      int               default 0,
   CountUntested           int               default 0,
   MinInGroup              bigint            default 0,
   MaxInGroup              bigint            default 0,
   CompletedThru           bigint            default 0,
   LeadingEdge             bigint            default 0,
   PRPandPrimesFound       int               default 0,
   SierpinskiRieselPrimeN  int               default 0 
) ENGINE=InnoDB;

create table WWWWRange (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   CompletedTests          int               default 0 not null,
   HasPendingTest          int               default 0 not null,
   DoubleChecked           int               default 0 not null,
   LastUpdateTime          bigint,
   primary key (LowerLimit, UpperLimit),
   index ix_processed (CompletedTests,HasPendingTest,DoubleChecked),
   index ix_pendging_range (HasPendingTest,LowerLimit,UpperLimit)
) ENGINE=InnoDB;

create table WWWWRangeTest (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   TestID                  bigint            not null,
   EmailID                 varchar(200)      collate latin1_bin,
   UserID                  varchar(200)      collate latin1_bin,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   SecondsToTestRange      double            default 0,
   SearchingProgram        varchar(50)       collate latin1_bin,
   SearchingProgramVersion varchar(50)       collate latin1_bin,
   PrimesTested            bigint,
   CheckSum                varchar(30),
   primary key (LowerLimit, UpperLimit, TestID),
   foreign key (LowerLimit, UpperLimit) references WWWWRange (LowerLimit, UpperLimit) on delete restrict,
   index ix_userid (UserID),
   index ix_teamid (TeamID)
) ENGINE=InnoDB;

create table WWWWRangeTestResult (
   LowerLimit              bigint         not null,
   UpperLimit              bigint         not null,
   TestID                  bigint         not null,
   Prime                   bigint         not null,
   Remainder               int            not null,
   Quotient                int,
   Duplicate               int            not null,
   EmailSent               int,
   primary key (LowerLimit, UpperLimit, TestID, Prime),
   foreign key (LowerLimit, UpperLimit, TestID) references WWWWRangeTest (LowerLimit, UpperLimit, TestID) on delete restrict
) ENGINE=InnoDB;

create table WWWWGroupStats (
   CountInGroup            int            default 0,
   CountInProgress         int            default 0,
   CountTested             int            default 0,
   CountDoubleChecked      int            default 0,
   CountUntested           int            default 0,
   MinInGroup              bigint         default 0,
   MaxInGroup              bigint         default 0,
   CompletedThru           bigint         default 0,
   LeadingEdge             bigint         default 0,
   WWWWPrimes              int            default 0,
   NearWWWWPrimes          int            default 0
) ENGINE=InnoDB;
