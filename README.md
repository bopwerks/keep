# keep

`keep` is a language for tracking finances using the double-entry bookkeeping method. The user defines accounts with the `account` keyword, creates hierarchies of accounts with the `<-` operator, and lists the transactions as seen in an accounting journal.

The software currently only parses the input journal, displays the balance of each account in a tree format, and terminates. In the future, users will be able to define mathematical expressions as functions of the balances of the different accounts which can be `track`ed over different periods of time, and the software will automatically produce reports from the information tracked.

## Example

```
$ cat journal
account income "Income"
income <- account job "Net Income"

account assets "Assets"
assets <- account checking "Bank Checking" dr 500.00
assets <- account savings "Bank Savings" dr 10000.00
assets <- account cash "Cash"
assets <- account accts-recv "Accounts Receivable"

account expenses "Expenses"
expenses <- account auto "Auto"
expenses <- account food "Food"
expenses <- account rent "Rent"

2020 may 1 "Paycheck"
    checking dr 2000.00
    job      cr 2000.00;
2020 may 1 "Rent Payment"
    rent      dr 1000.00
    checking  cr 1000.00;
2020 may 2 "Groceries"
    food  dr 50.00
    cash  cr 50.00;
$ ./keep <journal
Income 2000.00
	Net Income 2000.00
Assets 11450.00
	Bank Checking 1500.00
	Bank Savings 10000.00
	Cash 50.00
	Accounts Receivable 0.00
Expenses 1050.00
	Auto 0.00
	Food 50.00
	Rent 1000.00
$ 
```
