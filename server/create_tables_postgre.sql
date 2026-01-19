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
drop table GFNDivisor cascade;
drop table GFNDRangeTest cascade;
drop table GFNDRange cascade;
drop table DMDivisor cascade;
drop table DMDRangeTest cascade;
drop table DMDRange cascade;


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
   UserID                  varchar(200),
   Prime                   bigint            not null,
   Remainder               int,
   Quotient                int,
   MachineID               varchar(200),
   InstanceID              varchar(200),
   TeamID                  varchar(200),
   DateReported            bigint            not null,
   ShowOnWebPage           int
);

create unique index pk_UserWWWWs on UserWWWWs (UserID, Prime);

create table UserPrimes (
   UserID                  varchar(200)      not null,
   CandidateName           varchar(50)       not null,
   TestedNumber            varchar(50)       not null,
   TestResult              int               default 1,
   MachineID               varchar(200),
   InstanceID              varchar(200),
   TeamID                  varchar(200),
   DecimalLength           double precision,
   DateReported            bigint            not null,
   ShowOnWebPage           int,
   k                       bigint,
   b                       int,
   n                       int,
   c                       bigint.
   d                       int
);
   
create unique index pk_UserPrimes on UserPrimes (UserID, CandidateName, TestedNumber);

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
   DoubleChecked           int   default 0   not null,
   k                       bigint,
   b                       int,
   n                       int,
   c                       bigint,
   d                       int,
   MainTestResult          int,
   LastUpdateTime          bigint            not null
);

create unique index pk_Candidate on Candidate (CandidateName);
create index ix_bkcn on Candidate (b, k, c, n);
create index ix_bnck on Candidate (b, n, c, k);
create index ix_kn on Candidate k, n),
create index ix_nk on Candidate (n, k),
create index ix_completed on Candidate (CompletedTests);
create index ix_length on Candidate (DecimalLength);

create table CandidateTest (
   CandidateName           varchar(50)       not null,
   TestID                  bigint            not null,
   IsCompleted             int   default 0   not null,
   EmailID                 varchar(200),
   UserID                  varchar(200),
   MachineID               varchar(200),
   InstanceID              varchar(200),
   TeamID                  varchar(200),
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
   MachineID               varchar(200),
   InstanceID              varchar(200),
   TeamID                  varchar(200),
   foreign key (CandidateName) references Candidate (CandidateName) on delete restrict
);

create unique index pk_CandidateGFNDivisor on CandidateGFNDivisor (CandidateName, GFN);
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
   c                       bigint            default 0,
   d                       int               default 0,
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
   DoubleChecked           int               default 0 not null,
   LastUpdateTime          bigint
);

create unique index pk_WWWWRange on WWWWRange (LowerLimit, UpperLimit);
create index ix_processed on WWWWRange (CompletedTests, HasPendingTest, DoubleChecked);
create unique index ix_pendging_range on WWWWRange (HasPendingTest, LowerLimit, UpperLimit);

create table WWWWRangeTest (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   TestID                  bigint            not null,
   EmailID                 varchar(200),
   UserID                  varchar(200),
   MachineID               varchar(200),
   InstanceID              varchar(200),
   TeamID                  varchar(200),
   SecondsToTestRange      double precision  default 0,
   SearchingProgram        varchar(50),
   SearchingProgramVersion varchar(50),
   PrimesTested            bigint,
   CheckSum                varchar(30),
   foreign key (LowerLimit, UpperLimit) references WWWWRange (LowerLimit, UpperLimit) on delete restrict
);

create unique index pk_WWWWRangeTest on WWWWRangeTest (LowerLimit, UpperLimit, TestID);
create index ix_WWWWRTuserid on WWWWRangeTest (UserID);
create index ix_WWWWRTteamid on WWWWRangeTest (TeamID);

create table WWWWRangeTestResult (
   LowerLimit              bigint            not null,
   UpperLimit              bigint            not null,
   TestID                  bigint            not null,
   Prime                   bigint            not null,
   Remainder               int               not null,
   Quotient                int,
   Duplicate               int               not null,
   EmailSent               int,
   foreign key (LowerLimit, UpperLimit, TestID) references WWWWRangeTest (LowerLimit, UpperLimit, TestID) on delete restrict
);

create unique index pk_WWWWRangeTestResult on WWWWRangeTestResult (LowerLimit, UpperLimit, TestID, Prime);

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

create table GFNDRange (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   rangeStatus             int               default 0 not null,
   divisors                int               default 0 not null,
   lastUpdateTime          bigint
);

create unique index pk_GFNDRange on GFNDRange (n, kMin);

create table GFNDRangeTest (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   testID                  bigint            not null,
   emailID                 varchar(200),
   userID                  varchar(200),
   machineID               varchar(200),
   instanceID              varchar(200),
   secondsToTestRange      double            default 0,
   searchingProgram        varchar(50),
   searchingProgramVersion varchar(50)
);

create unique index pk_GFNDRangeTest on GFNDRangeTest (n, kMin, testId);

create table GFNDivisor (
   divisor                 varchar(200),
   emailID                 varchar(200),
   userID                  varchar(200),
   machineID               varchar(200),
   instanceID              varchar(200),
   dateReported            bigint            not null
);

create unique index pk_GFNDivisor on GFNDivisor (divisor);

create table DMDRange (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   rangeStatus             int               default 0 not null,
   divisors                int               default 0 not null,
   lastUpdateTime          bigint,
);

create unique index pk_DMDRange on DMDRange (n, kMin);

create table DMDRangeTest (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   testID                  bigint            not null,
   emailID                 varchar(200),
   userID                  varchar(200),
   machineID               varchar(200),
   instanceID              varchar(200),
   secondsToTestRange      double            default 0,
   searchingProgram        varchar(50),
   searchingProgramVersion varchar(50)
);

create unique index pk_DMDRangeTest on DMDRangeTest (n, kMin, testId);

create table DMDivisor (
   divisor                 varchar(200),
   emailID                 varchar(200),
   userID                  varchar(200),
   machineID               varchar(200),
   instanceID              varchar(200),
   dateReported            bigint            not null
);

create unique index pk_DMDivisor on DMDivisor (divisor);
