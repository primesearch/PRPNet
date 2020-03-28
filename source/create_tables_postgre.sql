-- Read server\readme_tables.txt for comments and descriptions of these tables and columns
-- InnoDB must be used because PRPNet needs a database engine that supports transactions
-- latin1_bin is used because column values are case sensitive

drop table CandidateTestResult cascade;
drop table GeneferROE cascade;
drop table CandidateGFNDivisor cascade;
drop table CandidateGroupStats cascade;
drop table CandidateTest cascade;
drop table Candidate cascade;
drop table UserWWWWs cascade;
drop table UserStats cascade;
drop table UserPrimes cascade;
drop table UserTeamStats cascade;
drop table TeamStats cascade;
drop table WWWWGroupStats cascade;
drop table WWWWRangeTestResult cascade;
drop table WWWWRangeTest cascade;
drop table WWWWRange cascade;

create table UserStats (
   UserID                  varchar(200)      not null,
   TestsPerformed          int   default 0   not null,
   PRPsFound               int   default 0   not null,
   PrimesFound             int   default 0   not null,
   GFNDivisorsFound        int   default 0   not null,
   WWWWPrimes              int   default 0   not null,
   NearWWWWPrimes          int   default 0   not null,
   TotalScore              double precision
);

create unique index pk_UserStats on UserStats (UserID);

create table UserWWWWs (
   UserID                  varchar(200)      collate latin1_bin,
   Prime                   bigint            not null,
   Remainder               int,
   Quotient                int,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   DateReported            bigint            not null,
   ShowOnWebPage           int
);

create unique index pk_UserWWWWs on UserStats (UserWWWWs, Prime);

create table UserPrimes (
   UserID                  varchar(200)      not null,
   CandidateName           varchar(50)       not null,
   TestedNumber            varchar(50)       not null,
   TestResult              int               default 1,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   DecimalLength           double precision,
   DateReported            bigint            not null,
   ShowOnWebPage           int
);
   
create unique index pk_UserPrimes on UserPrimes (UserID, TestedNumber);

create table UserTeamStats (
   UserID                  varchar(200)      not null,
   TeamID                  varchar(200)      not null,
   TestsPerformed          int   default 0   not null,
   PRPsFound               int   default 0   not null,
   PrimesFound             int   default 0   not null,
   GFNDivisorsFound        int   default 0   not null,
   WWWWPrimes              int   default 0   not null,
   NearWWWWPrimes          int   default 0   not null,
   TotalScore              double precision
);

create unique index pk_UserTeamStats on UserTeamStats (UserID, TeamID);

create table TeamStats (
   TeamID                  varchar(200)      not null,
   TestsPerformed          int   default 0   not null,
   PRPsFound               int   default 0   not null,
   PrimesFound             int   default 0   not null,
   GFNDivisorsFound        int   default 0   not null,
   WWWWPrimes              int   default 0   not null,
   NearWWWWPrimes          int   default 0   not null,
   TotalScore              double precision
);

create unique index pk_TeamStats on TeamStats (TeamID);

create table Candidate (
   CandidateName           varchar(50)       not null,
   DecimalLength           double precision,
   CompletedTests          int   default 0   not null,
   HasPendingTest          int   default 0   not null,
   doubleChecked           int   default 0   not null,
   k                       bigint,
   b                       int,
   n                       int,
   c                       int,
   MainTestResult          int,
   LastUpdateTime          bigint            not null
);

create unique index pk_Candidate on Candidate (CandidateName);
create index ix_bkcn on Candidate (b, k, c, n);
create index ix_bnck on Candidate (b, n, c, k);
create index ix_completed on Candidate (CompletedTests);
create index ix_length on Candidate (DecimalLength);

create table CandidateTest (
   CandidateName           varchar(50)       not null,
   TestID                  bigint            not null,
   IsCompleted             int   default 0   not null,
   EmailID                 varchar(200),
   UserID                  varchar(200),
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   EmailSent               int,
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict
);

create unique index pk_CandidateTest on CandidateTest (CandidateName, TestID);

create index ix_CTUserID on CandidateTest (UserID);
create index ix_CTTeamID on CandidateTest (TeamID);

create table CandidateTestResult (
   CandidateName           varchar(50)       not null,
   TestID                  bigint            not null,
   TestIndex               int               not null,
   WhichTest               varchar(20),
   TestedNumber            varchar(50)       not null,
   TestResult              int   default 0   not null,
   Residue                 varchar(20),
   PRPingProgram           varchar(50),
   ProvingProgram          varchar(50),
   PRPingProgramVersion    varchar(50),
   ProvingProgramVersion   varchar(50),
   SecondsForTest          double precision  default 0,
   CheckedGFNDivisibility  int   default 0,
   foreign key (CandidateName, TestID) references CandidateTest (CandidateName, TestID) on delete restrict
);

create unique index pk_CandidateTestResult on CandidateTestResult (CandidateName, TestID, TestIndex);
create index ix_residue on CandidateTestResult (CandidateName, WhichTest, Residue);

create table CandidateGFNDivisor (
   CandidateName           varchar(50)       not null,
   TestedNumber            varchar(50)       not null,
   GFN                     varchar(50)       not null,
   EmailID                 varchar(200),
   UserID                  varchar(200),
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict
);

create index ix_UserID on CandidateGFNDivisor (UserID);

create table GeneferROE (
   CandidateName           varchar(50),
   GeneferVersion          varchar(20),
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict
);

create unique index pk_GeneferROE on GeneferROE (CandidateName, GeneferVersion);

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
);

create table WWWWRange (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   CompletedTests          int               default 0 not null,
   HasPendingTest          int               default 0 not null,
   DoubleChecked           int               default 0 not null
);

create unique index pk_WWWWRange on WWWWRange (LowPrime, HighPrime);
create unique index ix_pendging_range on WWWWRange (HasPendingTest, LowerLimit, UpperLimit);

create table WWWWRangeTest (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   TestID                  bigint            not null,
   IsCompleted             int               default 0 not null,
   EmailID                 varchar(200)      collate latin1_bin,
   UserID                  varchar(200)      collate latin1_bin,
   MachineID               varchar(200)      collate latin1_bin,
   InstanceID              varchar(200)      collate latin1_bin,
   TeamID                  varchar(200)      collate latin1_bin,
   EmailSent               int,
   SecondsToTestRange      double            default 0,
   SearchingProgram        varchar(50)       collate latin1_bin,
   SearchingProgramVersion varchar(50)       collate latin1_bin,
   foreign key (LowerLimit, UpperLimit) references WWWWRange (LowerLimit, UpperLimit) on delete restrict,
   index ix_userid (UserID),
   index ix_teamid (TeamID)
);

create unique index pk_WWWWRangeTest on WWWWRangeTest (LowerLimit, UpperLimit, TestID);
create index ix_userid on WWWWRangeTest (UserID);
create index ix_teamid on WWWWRangeTest (TeamID);
alter table WWWWRange add key ix_pendging_range (HasPendingTest,LowerLimit,UpperLimit);

create table WWWWRangeTestResult (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   TestID                  bigint            not null,
   Prime                   bigint            not null,
   Remainder               int               not null,
   Quotient                int,
   Duplicate               int               not null,
   foreign key (LowerLimit, UpperLimit, TestID) references WWWWRangeTest (LowerLimit, UpperLimit, TestID) on delete restrict,
);

create unique index pk_WWWWTestResult on WWWWTestResult (LowerLimit, UpperLimit, TestID, Prime);

create table WWWWGroupStats (
   CountInGroup            int               default 0,
   CountInProgress         int               default 0,
   CountTested             int               default 0,
   CountDoubleChecked      int               default 0,
   CountUntested           int               default 0,
   MinInGroup              bigint            default 0,
   MaxInGroup              bigint            default 0,
   CompletedThru           bigint            default 0,
   LeadingEdge             bigint            default 0,
   WWWWPrimes              int               default 0,
   NearWWWWPrimes          int               default 0
);

