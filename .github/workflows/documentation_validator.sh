# List all of the Grid LUA API function in text.txt
touch "test.txt"
grep -i "GRID_LUA_FNC_.*_human" "./grid-protocol/grid_protocol_bot.json" > test.txt

# Regexpr to only keep the human readable fnc names
sed -r -i 's/ *".*"://g' test.txt
sed -r -i 's/ "//g' test.txt
sed -r -i 's/",//g' test.txt

# Search function in documentation
file=$(cat test.txt)

pass=0
fail=0

for line in $file
do

    if ls | grep -q -ir "$line" "./grid-documentation"; then ((pass++)); else ((fail++)); fi

done

echo "Coverage: $((100*$pass/($pass+$fail)))%  ($pass Passed, $fail Failed)"

for line in $file
do

    if ls | grep -q -ir "$line" "./grid-documentation"; then pass=0; else echo "FAILED: $line"; fi

done
