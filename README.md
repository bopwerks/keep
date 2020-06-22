# keep

`keep` is a language for tracking finances using the double-entry bookkeeping method. The user defines accounts with the `asset`, `liability`, `income`, and `expense` keywords; creates hierarchies of accounts with the `<-` operator; displaying balance sheets and income statements; automatically computes values or ratios of interest using the `track` keyword; and produces plots of these accounts or values using the `plot` subcommand.

## Example

Input to `keep` is plain ASCII text, like source code. Account and "tracker" definitions come before the percent sign, and transactions come after. In this example we store input in a file called `journal`.
```
$ cat journal
asset checking "Checking" dr 100.00;
expense food "Food";
income salary "Paycheck";

track savings "Monthly Savings" income - expenses;
track savings_rate "Monthly Savings Rate" savings/income;

%

2020 jan 1 "Buy groceries"
	food 		dr 100.00
    checking	cr 100.00;
2020 jan 1 "Paycheck"
	salary	 	cr 1000.00
    checking	dr 1000.00;
2020 feb 1 "Buy groceries"
	food 		dr 200.00
    checking	cr 200.00;
2020 feb 1 "Paycheck"
	salary	 	cr 500.00
    checking	dr 500.00;
2020 mar 1 "Buy groceries"
	food 		dr 300.00
    checking	cr 300.00;
2020 mar 1 "Paycheck"
	salary	 	cr 900.00
    checking	dr 900.00;
2020 apr 1 "Buy groceries"
	food 		dr 400.00
    checking	cr 400.00;
2020 apr 1 "Paycheck"
	salary	 	cr 400.00
    checking	dr 400.00;
2020 may 1 "Buy groceries"
	food 		dr 500.00
    checking	cr 500.00;
2020 may 1 "Paycheck"
	salary	 	cr 800.00
    checking	dr 800.00;
```

The `track` keyword tells `keep` to compute the given expression whenever data is available, and to give the resulting sequence of values a name. This tracker variable can be used in other `track` expressions.

Accounts and tracker variables can be plotted. The `plot` subcommand takes the names of accounts or trackers and produces input to `gnuplot` on standard out. This output may be piped to `gnuplot` to produce a graphical plot.

```
$ ./keep journal plot income expenses savings | gnuplot -p
```

![The result of 'plot income expenses savings'](https://raw.githubusercontent.com/bopwerks/keep/master/example.png)
