alter table UserPrimes add k bigint;
alter table UserPrimes add b int;
alter table UserPrimes add n int;
alter table UserPrimes add c int;

update UserPrimes up set k = (select c.k from Candidate c where c.candidateName = up.candidateName);
update UserPrimes up set b = (select c.b from Candidate c where c.candidateName = up.candidateName);
update UserPrimes up set n = (select c.n from Candidate c where c.candidateName = up.candidateName);
update UserPrimes up set c = (select c.c from Candidate c where c.candidateName = up.candidateName);
