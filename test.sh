EXEC=./bfs
for ((i = 1; i <= 10; i++)); do
  target=result/web-Stanford-$i-1-0.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/web-Stanford.mm 1 0 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/web-Stanford-$i-1-2.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/web-Stanford.mm 1 2 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/roadNet-CA-$i-1-0.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/roadNet-CA.mm 1 0 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/roadNet-CA-$i-1-2.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/roadNet-CA.mm 1 2 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/soc-LiveJournal1-$i-1-0.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/soc-LiveJournal1.mm 1 0 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/soc-LiveJournal1-$i-1-2.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/soc-LiveJournal1.mm 1 2 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/com-orkut-$i-1-0.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/com-orkut.ungraph.txt 1 0 | tee $target
  fi
done

for ((i = 1; i <= 10; i++)); do
  target=result/com-orkut-$i-1-2.txt
  if [[ ! -e "$target" ]]; then
    $EXEC $i graph/com-orkut.ungraph.txt 1 2 | tee $target
  fi
done

for ((k = 1; k <= 3; k++)); do
  for ((i = 1; i <= 10; i++)); do
    target=result/RMAT$k-$i-1-0.txt
    if [[ ! -e "$target" ]]; then
      $EXEC $i graph/RMAT$k.txt 1 0 | tee $target
    fi
  done

  for ((i = 1; i <= 10; i++)); do
    target=result/RMAT$k-$i-1-2.txt
    if [[ ! -e "$target" ]]; then
      $EXEC $i graph/RMAT$k.txt 1 2 | tee $target
    fi
  done
done

is_top_down=('0' '2')
# Parallel
for top_down in "${is_top_down[@]}"; do
  for ((k = 2; k <= 8; k++)); do
    for ((i = 1; i <= 10; i++)); do
      target=result/web-Stanford-$i-$k-$top_down.txt
      if [[ ! -e "$target" ]]; then
        $EXEC $i graph/web-Stanford.mm $k $top_down | tee $target
      fi
    done
  done
done

for top_down in "${is_top_down[@]}"; do
  for ((k = 2; k <= 8; k++)); do
    for ((i = 1; i <= 10; i++)); do
      target=result/roadNet-CA-$i-$k-$top_down.txt
      if [[ ! -e "$target" ]]; then
        $EXEC $i graph/roadNet-CA.mm $k $top_down | tee $target
      fi
    done
  done
done

for top_down in "${is_top_down[@]}"; do
  for ((k = 2; k <= 8; k++)); do
    for ((i = 1; i <= 10; i++)); do
      target=result/soc-LiveJournal1-$i-$k-$top_down.txt
      if [[ ! -e "$target" ]]; then
        $EXEC $i graph/soc-LiveJournal1.mm $k $top_down | tee $target
      fi
    done
  done
done

for top_down in "${is_top_down[@]}"; do
  for ((k = 2; k <= 8; k++)); do
    for ((i = 1; i <= 10; i++)); do
      target=result/com-orkut-$i-$k-$top_down.txt
      if [[ ! -e "$target" ]]; then
        $EXEC $i graph/com-orkut.ungraph.txt $k $top_down | tee $target
      fi
    done
  done
done
