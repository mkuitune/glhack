
#cmd = "etags *"
cmd = "dir"

puts "Update tags in project. Will run command:'#{cmd}'"
result = %x[#{cmd}]
puts result

