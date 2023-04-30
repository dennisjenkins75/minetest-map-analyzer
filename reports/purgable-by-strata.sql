select
  count(*) as count,
  floor(mapblock_y * 16 / 1000) * 1000 as stratum,
  min(mapblock_y) as min_y,
  max(mapblock_y) as max_y
from blocks
where not preserve and not uniform
group by 2
order by 2 desc;
