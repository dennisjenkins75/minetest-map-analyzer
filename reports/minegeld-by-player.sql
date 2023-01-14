with raw_data as (
  select a.name, sum(l.minegeld) as minegeld
  from actor a, nodes l
  where (a.actor_id = l.owner_id)
  group by a.name
)
select name, minegeld from raw_data
where minegeld > 0
order by minegeld desc
limit 20;
