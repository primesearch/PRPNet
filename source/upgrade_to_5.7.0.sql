alter table Candidate add d int;
alter table CandidateGroupStats add d int;
alter table UserPrimes add d int;

update Candidate set d = 1;
update CandidateGroupStats set d = 1;
update UserPrimes set d = 1;