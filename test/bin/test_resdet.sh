[[ "$OS" == "Windows"* ]] && CR=$'\r'

test_no_arguments_prints_usage_and_fails() {
	cmd="resdet"

	assert_fails "$cmd"
	assert_matches "Usage: " "$($cmd 2>&1)"
}

test_help_prints_usage_and_succeeds() {
	cmd="resdet -h"

	assert "$cmd"
	assert_matches "Usage: " "$($cmd)"
}

test_prints_version() {
	output="\
resdet version [[:digit:]].[[:digit:]].[[:digit:]](\\+[a-f0-9]+)?$CR
libresdet version [[:digit:]].[[:digit:]].[[:digit:]](\\+[a-f0-9]+)?$"

	assert_matches "$output" "$(resdet -V)"
}

test_verbosity_level_0() {
	assert "resdet -v0 ../files/blue_marble_2012_resized.pfm"
}

test_verbosity_level_0_fails_if_no_resolutions_detected() {
	assert_fails "resdet -v0 ../files/checkerboard.pfm"
}

test_verbosity_level_1() {
	assert_equals "512 512" "$(resdet -v1 ../files/blue_marble_2012_resized.pfm)"
}

test_verbosity_level_2() {
	output="\
512 768 $CR
512 768 "

	assert_equals "$output" "$(resdet -v2 ../files/blue_marble_2012_resized.pfm)"
}

test_verbosity_level_3() {
	output="\
512:0\\.99[[:digit:]]+ 768:-1.000000 $CR
512:0\\.99[[:digit:]]+ 768:-1.000000 $"

	assert_matches "$output" "$(resdet -v3 ../files/blue_marble_2012_resized.pfm)"
}

default_output="\
given: 768x768$CR
best guess: 512x512$CR
all width        height$CR
  512 \\([[:digit:]]+\\.[[:digit:]]+%\\)     512 \\([[:digit:]]+\\.[[:digit:]]+%\\)"

test_verbosity_negative_1() {
	assert_matches "$default_output" "$(resdet -v-1 ../files/blue_marble_2012_resized.pfm)"
}

test_default_verbosity() {
	assert_matches "$default_output" "$(resdet ../files/blue_marble_2012_resized.pfm)"
}

test_all_detection_methods_detect_resolutions() {
	for method in sign mag orig; do
		assert_matches "$default_output" "$(resdet -m "$method" ../files/blue_marble_2012_resized.pfm)"
	done
}

test_no_resolutions_detected() {
	output="\
given: 2x2$CR
best guess: 2x2 (not upsampled)"

	assert_equals "$output" "$(resdet ../files/checkerboard.pfm)"
}

test_filetype_option_with_extension() {
	assert_equals "2 2" "$(resdet -v1 -t y4m ../files/checkerboard)"
}

test_filetype_option_with_mimetype() {
	assert_equals "2 2" "$(resdet -v1 -t video/yuv4mpeg ../files/checkerboard)"
}

test_detects_from_stdin_with_filetype_option() {
	assert_matches "$default_output" "$(resdet -t pfm - < ../files/blue_marble_2012_resized.pfm)"
}

test_prints_error_if_image_doesnt_exist() {
	cmd="resdet ../files/doesntexist.y4m"

	assert_fails "$cmd"
	assert_equals "No such file or directory" "$(resdet ../files/doesntexist.y4m 2>&1)"
}

test_corrupt_images_print_invalid_image_error() {
	cmd="resdet ../files/corrupt_header.y4m"

	assert_fails "$cmd"
	assert_equals "Invalid image" "$($cmd 2>&1)"
}

test_zero_threshold_prints_all_analyzed_resolutions() {
	assert_equals 744 "$(resdet -x0 ../files/blue_marble_2012_resized.pfm | grep -F -c %)"
}

test_invalid_threshold_prints_error() {
	cmd="resdet -x-1 ../files/blue_marble_2012_resized.pfm"

	assert_fails "$cmd"
	assert_equals "Invalid threshold value -1" "$($cmd 2>&1)"

	cmd="resdet -x101 ../files/blue_marble_2012_resized.pfm"

	assert_fails "$cmd"
	assert_equals "Invalid threshold value 101" "$($cmd 2>&1)"
}

test_default_range_gives_one_result() {
	assert_equals 1 "$(resdet ../files/blue_marble_2012_resized.pfm | grep -F -c %)"
}

test_lower_range_gives_more_results() {
	assert_equals 4 "$(resdet -r8 ../files/blue_marble_2012_resized.pfm | grep -F -c %)"
}

test_invalid_range_prints_error() {
	cmd="resdet -r0 ../files/blue_marble_2012_resized.pfm"

	assert_fails "$cmd"
	assert_equals "Invalid range value 0" "$($cmd 2>&1)"
}

test_progress_option_prints_frame_numbers() {
	assert_equals $'Analyzing frame 1\rAnalyzing frame 2\r' "$(resdet -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_nframes_option_limits_analysis() {
	assert_equals $'Analyzing frame 1\r' "$(resdet -n1 -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_non_numeric_nframes_prints_error() {
	cmd="resdet -nx ../files/checkerboard.y4m"

	assert_fails "$cmd"
	assert_equals "Invalid nframes x" "$($cmd 2>&1)"
}

test_offset_option_skips_frames() {
	output=$'Seeking past frame 1\r'"$CR"$'
Analyzing frame 2\r'

	assert_equals "$output" "$(resdet -o1 -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_offset_at_last_frame_prints_error() {
	output=$'Seeking past frame 1\rSeeking past frame 2\r'"$CR"$'
No frames left in input.'

	assert_equals "$output" "$(resdet -o2 -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_non_numeric_offset_prints_error() {
	assert_fails "resdet -ox ../files/checkerboard.y4m"
	assert_equals "Invalid offset x" "$(resdet -ox ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_offset_past_end_of_file_prints_error() {
	output=$'Seeking past frame 1\rSeeking past frame 2\rPassed end of file while seeking to frame 3'

	assert_fails "resdet -v1 -o3 -p ../files/checkerboard.y4m"
	assert_equals "$output" "$(resdet -o3 -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}

test_nframes_and_offset_work_together() {
	output=$'Seeking past frame 1\r'"$CR"$'\nAnalyzing frame 2\r'

	assert_equals "$output" "$(resdet -n1 -o1 -p ../files/checkerboard.y4m 2>&1 > /dev/null)"
}
