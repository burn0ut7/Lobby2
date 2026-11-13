using GCWorkbench.UI;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using System.IO;
static class Checks {
 static IEnumerable<T> Find<T>(DependencyObject root) where T:DependencyObject {
  if(root is T value)yield return value;
  foreach(var child in LogicalTreeHelper.GetChildren(root).OfType<DependencyObject>())foreach(var result in Find<T>(child))yield return result;
 }
 [STAThread] static void Main(){
  var app=new Application{ShutdownMode=ShutdownMode.OnExplicitShutdown};
  SynchronizationContext.SetSynchronizationContext(new DispatcherSynchronizationContext());
  var spec=new DialogSpec{Title="Update mods",PrimaryAction="Update selected",Fields=[new(){Id="lobby",Label="Lobby2",Kind="check",Enabled=false,Checked=true,Help="Up to date"},new(){Id="core",Label="GC Core",Kind="check",Checked=false,Help="Update available"}]};
  var window=new ToolWindow(spec,tools:true,observeStatus:false);
  var checks=Find<CheckBox>(window).ToArray();var update=Find<Button>(window).Single(b=>Equals(b.Content,"Update selected"));
  if(checks[0].IsEnabled||checks[0].IsChecked==true||checks[0].Opacity>=1||update.IsEnabled)throw new Exception("Current mod or empty action enabled");
  checks[1].IsChecked=true;if(!update.IsEnabled)throw new Exception("Available selected mod did not enable update");
  checks[1].IsChecked=false;if(update.IsEnabled)throw new Exception("Empty selection remained enabled");
  checks[0].IsChecked=true;checks[1].IsChecked=true;
  update.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(window.Result.Action!="update"||(bool)window.Result.Values["lobby"]||!(bool)window.Result.Values["core"])throw new Exception("Disabled mod leaked into dispatch");
  var setup=new ToolWindow(new DialogSpec{Title="External dependencies",PrimaryAction="Install / update",SecondaryAction="Close"},tools:true,observeStatus:false);
  if(setup.Result.Action!="cancel")throw new Exception("Setup started without user action");
  var busy=(HashSet<string>)typeof(ToolWindow).GetField("checking",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance)!.GetValue(setup)!;
  busy.Add("External dependencies");
  typeof(ToolWindow).GetMethod("Render",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.DeclaredOnly)!.Invoke(setup,[new DialogSpec{Title="External dependencies",PrimaryAction="Install / update"}]);
  var install=Find<Button>(setup).Single(b=>Equals(b.Content,"Install / update"));
  if(!install.IsEnabled)throw new Exception("Install blocked by background status check");
  install.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(setup.Result.Action!="install")throw new Exception("Install action unavailable after notice");
  var hub=new ToolWindow(spec,tools:true);
  Dictionary<string,object>? dispatched=null;
  hub.OperationRunner=(_,values,_)=>{dispatched=values;return Task.FromResult(new OperationResult("Updated"));};
  const System.Reflection.BindingFlags flags=System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.DeclaredOnly;
  var pages=(Dictionary<string,DialogSpec>)typeof(ToolWindow).GetField("statusPages",flags)!.GetValue(hub)!;
  var pending=(HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(hub)!;
  var render=typeof(ToolWindow).GetMethod("Render",flags)!;
  pages["Update mods"]=spec;
  var timer=System.Diagnostics.Stopwatch.StartNew();
  Find<Button>(hub).Single(b=>Equals(b.Content,"Submit mission")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Find<Button>(hub).Single(b=>Equals(b.Content,"Update mods")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(pending.Count!=0||Find<CheckBox>(hub).Count()!=2)throw new Exception("Navigation discarded cached results or restarted checks");
  Console.WriteLine($"Cached page round trip: {timer.Elapsed.TotalMilliseconds:F0} ms");
  pending.Add("Update mods");
  render.Invoke(hub,[spec]);
  var ready=Find<CheckBox>(hub).Last();ready.IsChecked=true;
  if(!Find<Button>(hub).Single(b=>Equals(b.Content,"Update selected")).IsEnabled)throw new Exception("Ready mod blocked by another pending check");
  typeof(ToolWindow).GetMethod("SaveDraft",flags)!.Invoke(hub,null);
  render.Invoke(hub,[spec]);
  if(Find<CheckBox>(hub).Last().IsChecked!=true)throw new Exception("Progress discarded user selection");
  Find<Button>(hub).Single(b=>Equals(b.Content,"Update selected")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(hub.Result.Action!="cancel"||dispatched is null||!(bool)dispatched["core"]||(bool)dispatched["lobby"])throw new Exception("Partial results dispatched unchecked or pending mods or closed hub");
  if(!Find<Button>(hub).Any(b=>Equals(b.Content,"Updated")))throw new Exception("Completion missing from action button");
  Console.WriteLine("PASS: cached navigation, partial-result action, selection retained across progress.");
  var submission=new ToolWindow(ToolPages.Submission(),tools:true);
  var completed=new TaskCompletionSource<OperationResult>();
  var reviewShown=false;
  submission.OperationRunner=async (_,_,review)=>{
   var answer=await review(new DialogSpec{Title="Review conflict",PrimaryAction="Use my file",SecondaryAction="Cancel"});
   if(answer.Action!="confirm")throw new Exception("Review did not return confirmation");
   reviewShown=true;
   return await completed.Task;
  };
  Find<Button>(submission).Single(b=>Equals(b.Content,"Review submission")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"Use my file")))throw new Exception("Review not embedded in hub");
  if(Find<Button>(submission).Single(b=>Equals(b.Content,"Update mods")).IsEnabled)throw new Exception("Navigation enabled during review");
  Find<Button>(submission).Single(b=>Equals(b.Content,"Use my file")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  Pump(()=>reviewShown);
  if(submission.Result.Action!="cancel")throw new Exception("Hub exited while workflow pending");
  completed.SetResult(new("Submitted","Awaiting approval","https://github.com/example/missions/pull/1"));
  Pump(()=>Find<Button>(submission).Any(b=>Equals(b.Content,"Submitted")));
  if(!Find<Button>(submission).Any(b=>Equals(b.Content,"View submission"))||submission.Result.Action!="cancel")throw new Exception("Submission result closed hub or lost link");
  Console.WriteLine("PASS: embedded conflict review, pending action, same-window submitted result and link.");
  var dependencies=new ToolWindow(ToolPages.Dependencies(),tools:true);
  var dependencyChecks=(HashSet<string>)typeof(ToolWindow).GetField("checking",flags)!.GetValue(dependencies)!;
  dependencyChecks.Add("External dependencies"); // Keep this test independent of network status checks.
  var attempts=0;
  var retried=new TaskCompletionSource<OperationResult>();
  dependencies.OperationRunner=(_,_,_)=>++attempts==1 ? Task.FromResult(new OperationResult("Installation stopped","Fixture error")) : retried.Task;
  Find<Button>(dependencies).Single(b=>Equals(b.Content,"Install / update")).RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  var retry=Find<Button>(dependencies).Single(b=>Equals(b.Content,"Retry"));
  var footer=(Panel)retry.Parent;
  if(footer.Children.Count!=1||Find<Button>(dependencies).Any(b=>Equals(b.Content,"Refresh")||Equals(b.Content,"Close")||Equals(b.Content,"Installation stopped")))throw new Exception("Failed dependencies must show only Retry in the footer");
  if(!Find<TextBlock>(dependencies).Any(t=>t.Text=="Fixture error"))throw new Exception("Retry lost error detail");
  retry.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
  if(attempts!=2||Find<Button>(dependencies).Single(b=>Equals(b.Content,"Installing...")).IsEnabled)throw new Exception("Retry did not run installation once with disabled progress button");
  retried.SetResult(new("Installed","Restart Workbench"));
  Pump(()=>Find<Button>(dependencies).Any(b=>Equals(b.Content,"Install / update")));
  if(dependencies.Result.Action!="cancel")throw new Exception("Retry closed tools");
  Console.WriteLine("PASS: single Retry button, visible error, real retry dispatch, disabled progress and persistent completion.");
  var fixture=Path.Combine(Path.GetTempPath(),"gc-hub-test-"+Guid.NewGuid()+".ps1");
  var reviewPath=Path.GetFullPath("external/source/tools/GC-Review.ps1").Replace("'","''");
  File.WriteAllText(fixture,". '"+reviewPath+"'\n$answer=Show-GcReview @{Title='Fixture review';PrimaryAction='Confirm'}\nif($answer.Action -ne 'confirm'){throw 'Wrong review answer'}\nSet-GcOutcome 'Submitted' 'Fixture confirmed' 'https://github.com/example/missions/pull/1'\n");
  var oldScript=Environment.GetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT");
  try {
   Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",fixture);
   var operation=ToolOperations.RunAsync("install",[],request=>Task.FromResult(new DialogResult("confirm",[])));
   Pump(()=>operation.IsCompleted);
   var receipt=operation.GetAwaiter().GetResult();
   if(receipt.Status!="Submitted"||receipt.Detail!="Fixture confirmed")throw new Exception("Pipe workflow did not return confirmed result");
   Console.WriteLine("PASS: real Windows PowerShell review pipe and operation outcome, without external mutations.");
  } finally {Environment.SetEnvironmentVariable("GC_WORKBENCH_INSTALL_SCRIPT",oldScript);File.Delete(fixture);}
  Console.WriteLine("PASS: current mods gray/unchecked/disabled, button follows eligible selection, disabled mods excluded from dispatch.");
 }
 static void Pump(Func<bool> done){
  var deadline=DateTime.UtcNow.AddSeconds(20);
  while(!done()){
   if(DateTime.UtcNow>deadline)throw new Exception("UI test timed out");
   var frame=new DispatcherFrame();
   Dispatcher.CurrentDispatcher.BeginInvoke(DispatcherPriority.Background,new Action(()=>frame.Continue=false));
   Dispatcher.PushFrame(frame);
   Thread.Sleep(5);
  }
 }
}
