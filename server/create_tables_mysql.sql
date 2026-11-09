-- Read server\readme_tables.txt for comments and descriptions of these tables and columns
-- InnoDB must be used because PRPNet needs a database engine that supports transactions
-- latin1_bin is used because column values are case sensitive

drop table if exists CandidateTestResult;
drop table if exists GeneferROE;
drop table if exists CandidateGFNDivisor;
drop table if exists candidateGroupStats;
drop table if exists CandidateTest;
drop table if exists Candidate;
drop table if exists UserStats;
drop table if exists UserPrimes;
drop table if exists UserTeamStats;
drop table if exists TeamStats;
drop table if exists GFNDivisor;
drop table if exists GFNDRangeTest;
drop table if exists GFNDRange;
drop table if exists DMDRangeTest;
drop table if exists DMDivisor;
drop table if exists DMDRange;

create table UserStats (
   userId                  varchar(200)   collate latin1_bin,
   testsPerformed          int            default 0 not null,
   prpsFound               int            default 0 not null,
   primesFound             int            default 0 not null,
   gfnDivisorsFound        int            default 0 not null,
   totalScore              double,
   primary key (userId)
) ENGINE=InnoDB;

create table UserPrimes (
   userId                  varchar(200)      collate latin1_bin,
   candidateName           varchar(50)       not null,
   testedNumber            varchar(50)       not null,
   testResult              int               default 1,
   machineId               varchar(200)      collate latin1_bin,
   instanceID              varchar(200)      collate latin1_bin,
   teamId                  varchar(200)      collate latin1_bin,
   decimalLength           double,
   dateReported            bigint            not null,
   showOnWebPage           int,
   k                       bigint,
   b                       int,
   n                       int,
   c                       bigint,
   d                       int,
   primary key (userId, candidateName, TestedNumber)
) ENGINE=InnoDB;

create table UserTeamStats (
   userId                  varchar(200)      collate latin1_bin,
   teamId                  varchar(200)      collate latin1_bin,
   testsPerformed          int               default 0 not null,
   prpsFound               int               default 0 not null,
   primesFound             int               default 0 not null,
   gfnDivisorsFound        int               default 0 not null,
   totalScore              double,
   primary key (userId, teamId)
) ENGINE=InnoDB;

create table TeamStats (
   teamId                  varchar(200)      collate latin1_bin,
   testsPerformed          int               default 0 not null,
   prpsFound               int               default 0 not null,
   primesFound             int               default 0 not null,
   gfnDivisorsFound        int               default 0 not null,
   totalScore              double,
   primary key (teamId)
) ENGINE=InnoDB;

create table Candidate (
   candidateName           varchar(50)       not null,
   candidateGroupId        varchar(200),
   decimalLength           double,
   completedTests          int               default 0 not null,
   hasPendingTest          int               default 0 not null,
   doubleChecked           int               default 0 not null,
   k                       bigint,
   b                       int,
   n                       int,
   c                       bigint,
   d                       int,
   mainTestResult          int,
   lastUpdateTime          bigint            not null,
   primary key (candidateName),
   index ix_bkcn (b, k, c, n),
   index ix_bnck (b, n, c, k),
   index ix_kn (k, n),
   index ix_nk (n, k),
   index ix_completed (completedTests),
   index ix_length (decimalLength),
   index ix_group (candidateGroupId)
) ENGINE=InnoDB;

create table CandidateTest (
   candidateName           varchar(50)       not null,
   testId                  bigint            not null,
   isCompleted             int               default 0 not null,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceID              varchar(200)      collate latin1_bin,
   teamId                  varchar(200)      collate latin1_bin,
   emailSent               int,
   primary key (candidateName, testId),
   foreign key (candidateName) references Candidate (candidateName) on delete restrict,
   index ix_userId (userId),
   index ix_teamId (teamId)
) ENGINE=InnoDB;

create table CandidateTestResult (
   candidateName           varchar(50)       not null,
   testId                  bigint            not null,
   testIndex               int               not null,
   whichTest               varchar(20)       collate latin1_bin,
   testedNumber            varchar(50)       not null,
   testResult              int               default 0 not null,
   residue                 varchar(20)       collate latin1_bin,
   prpingProgram           varchar(50)       collate latin1_bin,
   provingProgram          varchar(50)       collate latin1_bin,
   prpingProgramVersion    varchar(50)       collate latin1_bin,
   provingProgramVersion   varchar(50)       collate latin1_bin,
   secondsForTest          double            default 0,
   checkedGFNDivisibility  int               default 0,
   primary key (candidateName, testId, testIndex),
   foreign key (candidateName, testId) references CandidateTest (candidateName, testId) on delete restrict,
   index ix_residue (candidateName, WhichTest, Residue)
) ENGINE=InnoDB;

create table CandidateGFNDivisor (
   candidateName           varchar(50)       not null,
   testedNumber            varchar(50)       not null,
   gfn                     varchar(50)       not null,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceID              varchar(200)      collate latin1_bin,
   teamId                  varchar(200)      collate latin1_bin,
   primary key (candidateName, gfn),
   index ix_userId (userId),
   foreign key (candidateName) references Candidate (candidateName) on delete restrict
) ENGINE=InnoDB;

create table GeneferROE (
   candidateName           varchar(50),
   geneferVersion          varchar(20) collate latin1_bin,
   primary key (candidateName, geneferVersion),
   foreign key (candidateName) references Candidate (candidateName) on delete restrict
) ENGINE=InnoDB;

create table CandidateGroupStats (
   candidateGroupId        varchar(200)      not null,
   k                       bigint            default 0,
   b                       int               default 0,
   n                       int               default 0,
   c                       bigint            default 0,
   d                       int               default 0,
   countInGroup            int               default 0,
   countInProgress         int               default 0,
   countTested             int               default 0,
   countDoubleChecked      int               default 0,
   countUntested           int               default 0,
   minInGroup              bigint            default 0,
   maxInGroup              bigint            default 0,
   completedThru           bigint            default 0,
   leadingEdge             bigint            default 0,
   prpsAndPrimesFound      int               default 0,
   sierpinskiRieselPrimeN  int               default 0,
   primary key (candidateGroupId)
) ENGINE=InnoDB;

create table GFNDRange (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   rangeStatus             int               default 0 not null,
   divisors                int               default 0 not null,
   lastUpdateTime          bigint,
   primary key (n, kMin)
) ENGINE=InnoDB;

create table GFNDRangeTest (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   testId                  bigint            not null,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceId              varchar(200)      collate latin1_bin,
   secondsToTestRange      double            default 0,
   searchingProgram        varchar(50)       collate latin1_bin,
   searchingProgramVersion varchar(50)       collate latin1_bin,
   primary key (n, kMin, testId),
   foreign key (n, kMin) references GFNDRange (n, kMin) on delete restrict
) ENGINE=InnoDB;

create table GFNDivisor (
   divisor                 varchar(200)      collate latin1_bin,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceId              varchar(200)      collate latin1_bin,
   dateReported            bigint            not null,
   primary key (divisor)
) ENGINE=InnoDB;

create table DMDRange (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   divisors                int               default 0 not null,
   rangeStatus             int               default 0 not null,
   lastUpdateTime          bigint,
   primary key (n, kMin)
) ENGINE=InnoDB;

create table DMDRangeTest (
   n                       int               not null,
   kMin                    bigint            not null,
   kMax                    bigint            not null,
   testId                  bigint            not null,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceId              varchar(200)      collate latin1_bin,
   secondsToTestRange      double            default 0,
   searchingProgram        varchar(50)       collate latin1_bin,
   searchingProgramVersion varchar(50)       collate latin1_bin,
   primary key (n, kMin, testId),
   foreign key (n, kMin) references DMDRange (n, kMin) on delete restrict
) ENGINE=InnoDB;

create table DMDivisor (
   divisor                 varchar(200)      collate latin1_bin,
   emailId                 varchar(200)      collate latin1_bin,
   userId                  varchar(200)      collate latin1_bin,
   machineId               varchar(200)      collate latin1_bin,
   instanceId              varchar(200)      collate latin1_bin,
   dateReported            bigint            not null,
   primary key (divisor)
) ENGINE=InnoDB;