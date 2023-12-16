Project for Secure Coding.


Create code necessary to find and update user scores in a locally stored file whilst ensuring no vulnerabilities are introduced in the application.

This involves avoiding dangerous functions, such as those that use the command shell, or functions that don't specify the size of a buffer (which can be exploited to cause overflow), 
as well as ensuring data types with enough memory are used so that expected values don't cause an overflow. 
The specification requires that the program needs higher privileges to operate correctly, to prevent malicious use, the principle of least privilege is applied.

If any errors occur, gracefully stop the program and write an error message in the parameter passed to the function.
