BEGINFILE {
	match(FILENAME,/\/([^/]+)\.c/,matches)
	group = matches[1]
	ntests = 0
}

/^\/\/ group:/ { group = $3 }
/^\/\/ setup:/ { setup = $3 }
/^\/\/ teardown:/ { teardown = $3 }
/^\/\/ guard:/ { guard = $3 }
/^}$/ { setup = teardown = guard = "" }

/^void test_/ {
	match($0,/^void (test_[^(]+)/,matches)
	printf("void %s(void** state);\n",matches[1]);
	test = matches[1]

	testcmd = ""
	if(guard != current_guard) {
		if(current_guard)
			testcmd = testcmd "#endif\n"
		if(guard)
			testcmd = testcmd "#if "guard"\n"
	}
	current_guard = guard

	if(setup || teardown)
		testcmd = testcmd sprintf("\t\tcmocka_unit_test_setup_teardown(%s,%s,%s),",test,setup ? setup : "NULL",teardown ? teardown : "NULL")
	else
		testcmd = testcmd sprintf("\t\tcmocka_unit_test(%s),",test)

	tests[group][ntests] = testcmd
	ntests++
}

/^int (setup|teardown)_/ {
	match($0,/^int ((setup|teardown)[^(]*)/,matches)
	printf("int %s(void** state);\n",matches[1]);
}

/^int setup_.+_group/ {
	match($0,/^int (setup_.+_group[^(]*)/,matches)
	printf("int %s(void** state);\n",matches[1]);
	setups[group] = matches[1]
}

/^int teardown_.+_group/ {
	match($0,/^int (teardown_.+_group[^(]*)/,matches)
	printf("int %s(void** state);\n",matches[1]);
	teardowns[group] = matches[1]
}

END {
	print("\n#include <stddef.h>\n#include <setjmp.h>\n#include <stdarg.h>\n#include <cmocka.h>\n\nint main(void) {")

	for(group in tests) {
		printf("\tstruct CMUnitTest %s[] = {\n",group)
		for(i in tests[group])
			printf("%s\n",tests[group][i])
		print("\t};")
	}

	print("\n\n\tint ret = 0;\n")
	for(group in tests)
		printf("\tret |= cmocka_run_group_tests(%s,%s,%s);\n",group,setups[group] ? setups[group] : "NULL",teardowns[group] ? teardowns[group] : "NULL")

	print("\n\treturn ret;\n}")
}