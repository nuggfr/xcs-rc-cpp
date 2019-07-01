# xcs-rc cpp
Accuracy-based Learning Classifier Systems with Rule Combining mechanism (XCS-RC) in CPP

Read my PhD thesis <a href="https://publikationen.bibliothek.kit.edu/1000046880">here</a> for a complete algorithmic description.

A new functions called <i>combining</i> that employs <i>inductive reasoning</i> replaces all Darwinian genetic operation like mutation and crossover. My earlier papers comparing them can be obtained <a href="https://link.springer.com/chapter/10.1007/978-3-642-17298-4_30">here</a> and <a href="https://dl.acm.org/citation.cfm?id=2331009">here</a>.

New parameters in XCS-RC:
<ul>
  <li><i>Tcomb</i>: combining period, after how many learning cycles the new technique will be applied</li>
  <li><i>predTol</i>: the maximum difference between two classifiers to be combined
  <li><I>predErrTol</i>: threshold for rule deletion, indicated inappropriate combining
</ul>

Removed/unused parameters: <b>all</b> related to mutation and crossover.

This algorithm in Java available <a href="https://github.com/nuggfr/xcs-rc-java">here</a>.<br>
Should there be questions, feel free to ask. &#128578;
