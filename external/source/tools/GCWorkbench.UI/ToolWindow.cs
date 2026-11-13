using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Shell;

namespace GCWorkbench.UI;

public sealed class ToolWindow : Window
{
    private readonly bool preview;
    private readonly bool tools;
    private readonly bool observeStatus;
    private readonly Dictionary<string, Dictionary<string, object>> drafts = [];
    private readonly Grid body = new();
    private readonly Dictionary<string, FrameworkElement> inputs = [];
    private readonly Dictionary<string, TextBlock> errors = [];
    private readonly List<(Button Button, DialogSpec Spec)> navigation = [];
    private readonly CancellationTokenSource lifetime = new();
    private readonly Dictionary<string, DialogSpec> statusPages = [];
    private readonly HashSet<string> checking = [];
    private Button? refreshButton;
    private DialogSpec spec;
    private bool operating;
    private TaskCompletionSource<DialogResult>? reviewAnswer;
    private readonly Dictionary<string, OperationResult> outcomes = [];
    internal Func<string, Dictionary<string, object>, Func<DialogSpec, Task<DialogResult>>, Task<OperationResult>> OperationRunner;
    public DialogResult Result { get; private set; } = new("cancel", []);

    public ToolWindow(DialogSpec specification, bool preview = false, bool tools = false, bool observeStatus = true, bool dependenciesMissing = false)
    {
        spec = specification;
        this.preview = preview;
        this.tools = tools;
        this.observeStatus = observeStatus;
        OperationRunner = (action, values, review) => ToolOperations.RunAsync(action, values, review, new Progress<DependencyProgress>(UpdateDependencyProgress));
        Title = "GC Workbench";
        Icon = System.Windows.Media.Imaging.BitmapFrame.Create(new Uri("pack://application:,,,/GCWorkbench.UI;component/Assets/GC.ico"));
        var workArea = SystemParameters.WorkArea;
        Width = Math.Min(preview || tools ? 1180 : 900, workArea.Width);
        Height = Math.Min(preview || tools ? 800 : 720, workArea.Height);
        MinWidth = Math.Min(preview || tools ? 760 : 540, workArea.Width);
        MinHeight = Math.Min(500, workArea.Height);
        WindowStartupLocation = WindowStartupLocation.CenterScreen;
        FontFamily = new FontFamily("Segoe UI");
        FontSize = 14;
        Background = Brush("#191e18");
        Foreground = Brush("#f0e9d5");
        UseLayoutRounding = true;
        Resources = (ResourceDictionary)XamlReader.Parse(Styles);
        WindowStyle = WindowStyle.None;
        WindowChrome.SetWindowChrome(this, new WindowChrome { CaptionHeight = 40, ResizeBorderThickness = new Thickness(6), GlassFrameThickness = new Thickness(0), CornerRadius = new CornerRadius(0) });
        var shell = new Grid { Background = Background };
        shell.RowDefinitions.Add(new RowDefinition { Height = new GridLength(40) });
        shell.RowDefinitions.Add(new RowDefinition());
        var chrome = new DockPanel { LastChildFill = false, Background = Background };
        var controls = new StackPanel { Orientation = Orientation.Horizontal };
        foreach (var (label, name, action) in new (string, string, Action)[] {
            ("\u2212", "Minimize", () => WindowState = WindowState.Minimized),
            ("\u25a1", "Maximize or restore", () => WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized),
            ("\u00d7", "Close", Close) })
        {
            var control = new Button { Content = label, Width = 44, MinHeight = 0, Padding = new Thickness(0), Background = Brushes.Transparent, BorderThickness = new Thickness(0), FontSize = 18 };
            AutomationProperties.SetName(control, name);
            WindowChrome.SetIsHitTestVisibleInChrome(control, true);
            control.Click += (_, _) => action();
            controls.Children.Add(control);
        }
        DockPanel.SetDock(controls, Dock.Right);
        chrome.Children.Add(controls);
        var brand = new Border
        {
            Width = preview || tools ? 216 : 200, Background = preview || tools ? Brush("#242b20") : Background,
            Child = Text(preview ? "GC TOOLS / PREVIEW" : "GC TOOLS", 12, "#f0e9d5", bold: true, margin: new Thickness(24, 12, 0, 0))
        };
        DockPanel.SetDock(brand, Dock.Left);
        chrome.Children.Add(brand);
        shell.Children.Add(chrome);
        var root = new Grid { Background = Background };
        Grid.SetRow(root, 1);
        shell.Children.Add(root);
        if (preview || tools)
        {
            root.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(216) });
            root.ColumnDefinitions.Add(new ColumnDefinition());
            root.Children.Add(PreviewNavigation());
            Grid.SetColumn(body, 1);
        }
        root.Children.Add(body);
        Content = shell;
        Render(spec);
        if (tools && observeStatus)
        {
            bool initialized = false;
            ContentRendered += async (_, _) =>
            {
                if (initialized) return;
                initialized = true;
                bool missing = dependenciesMissing || await ToolStatus.MissingCredentialManagerAsync(lifetime.Token);
                if (lifetime.IsCancellationRequested || operating || reviewAnswer is not null) return;
                if (missing)
                {
                    Render(ToolPages.Dependencies());
                    await ReviewAsync(new DialogSpec { Title = "External dependencies not found", PrimaryAction = "OK", SecondaryAction = "" });
                    Render(ToolPages.Dependencies());
                    if (!lifetime.IsCancellationRequested) await RefreshStatusAsync(false, "External dependencies");
                }
                else await Task.WhenAll(
                    RefreshStatusAsync(false, "GitHub"),
                    RefreshStatusAsync(false, "Update mods"),
                    RefreshStatusAsync(false, "External dependencies"));
            };
        }
        Closed += (_, _) => lifetime.Cancel();
        Closing += (_, e) => { if (operating || reviewAnswer is not null) e.Cancel = true; };
        PreviewKeyDown += (_, e) => { if (e.Key == Key.Escape) { Close(); e.Handled = true; } };
    }

    private FrameworkElement PreviewNavigation()
    {
        var panel = new StackPanel { Margin = new Thickness(12, 24, 12, 16) };
        foreach (var (name, example) in (tools ? ToolPages.All() : Examples.All()))
        {
            var label = name switch { "dependencies" => "Dependencies", "updates" => "Update mods", "github" => "GitHub", _ => "Submit mission" };
            var button = new Button
            {
                Content = label, HorizontalContentAlignment = HorizontalAlignment.Left,
                Background = Brushes.Transparent, Foreground = Brush("#f0e9d5"), BorderBrush = Brushes.Transparent,
                Margin = new Thickness(0, 0, 0, 6), Padding = new Thickness(12, 14, 12, 14)
            };
            button.Click += async (_, _) =>
            {
                SaveDraft();
                Render(statusPages.GetValueOrDefault(example.Title, example));
                if (tools && observeStatus && !statusPages.ContainsKey(example.Title) && example.Title is "GitHub" or "External dependencies" or "Update mods") await RefreshStatusAsync();
            };
            navigation.Add((button, example));
            panel.Children.Add(button);
        }
        var sidebar = new Grid();
        sidebar.RowDefinitions.Add(new RowDefinition());
        sidebar.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        sidebar.Children.Add(panel);
        if (tools)
        {
            refreshButton = new Button { Content = "Refresh", MinHeight = 52, FontSize = 16, Margin = new Thickness(12, 12, 12, 24) };
            AutomationProperties.SetHelpText(refreshButton, "Refresh the selected tool's status");
            refreshButton.Click += async (_, _) =>
            {
                if (operating || reviewAnswer is not null) return;
                await RefreshStatusAsync(false, RefreshTarget());
            };
            Grid.SetRow(refreshButton, 1);
            sidebar.Children.Add(refreshButton);
        }
        return new Border { Background = Brush("#242b20"), Child = sidebar };
    }

    private string RefreshTarget() => spec.Title == "Submit mission" ? "GitHub" : spec.Title;

    private void UpdateRefreshButton()
    {
        if (refreshButton is not null)
            refreshButton.IsEnabled = !operating && reviewAnswer is null && !checking.Contains(RefreshTarget());
    }

    private void Render(DialogSpec next)
    {
        foreach (var (button, _) in navigation) button.IsEnabled = !operating && reviewAnswer is null;
        next.Validate();
        spec = next;
        UpdateRefreshButton();
        Resources["GcAccent"] = Brush(spec.Accent);
        foreach (var (button, page) in navigation)
        {
            bool selected = page.Title == spec.Title;
            button.Background = Brush(selected ? "#191e18" : "#242b20");
            button.BorderBrush = selected ? Brush(spec.Accent) : Brushes.Transparent;
            button.BorderThickness = new Thickness(3, 0, 0, 0);
            AutomationProperties.SetHelpText(button, selected ? "Selected" : "");
        }
        inputs.Clear();
        errors.Clear();
        body.Children.Clear();
        body.RowDefinitions.Clear();
        body.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        body.RowDefinitions.Add(new RowDefinition());
        body.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

        var header = new StackPanel { Margin = new Thickness(24, 24, 24, 16) };
        header.Children.Add(Text(spec.Title, 28, "#f0e9d5", bold: true, margin: new Thickness(0, 0, 0, 8)));
        if (spec.Description.Length > 0) header.Children.Add(Text(spec.Description, 14, "#a9b09b"));
        body.Children.Add(header);

        var content = new StackPanel { Margin = new Thickness(24, 0, 24, 16) };
        if (spec.Section.Length > 0) content.Children.Add(Text(spec.Section, 14, "#f0e9d5", bold: true, margin: new Thickness(0, 0, 0, 12)));
        foreach (var item in spec.Items) content.Children.Add(StatusCard(item));
        foreach (var field in spec.Fields)
        {
            var restored = field;
            if (drafts.TryGetValue(spec.Title, out var draft) && draft.TryGetValue(field.Id, out var value))
                restored = field.Kind == "check" ? field with { Checked = field.Enabled && (bool)value } : field with { Value = (string)value };
            content.Children.Add(Field(restored));
        }
        var feedback = Text("", 13, "#a9b09b", margin: new Thickness(0, 14, 0, 0));
        feedback.Visibility = Visibility.Collapsed;
        AutomationProperties.SetLiveSetting(feedback, AutomationLiveSetting.Polite);
        content.Children.Add(feedback);
        var scroll = new ScrollViewer { Content = content, VerticalScrollBarVisibility = ScrollBarVisibility.Auto, HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled };
        Grid.SetRow(scroll, 1);
        body.Children.Add(scroll);

        var footer = new StackPanel { Margin = new Thickness(24, 16, 24, 24) };
        if (spec.Note.Length > 0) footer.Children.Add(Text(spec.Note, 12, "#a9b09b", margin: new Thickness(0, 0, 0, 18)));
        var buttons = new WrapPanel { HorizontalAlignment = HorizontalAlignment.Right };
        var dependencyPage = spec.Title == "External dependencies" && reviewAnswer is null;
        var cancel = new Button { Content = spec.SecondaryAction, MinWidth = 92, Margin = new Thickness(0, 0, 10, 0) };
        cancel.Click += (_, _) => { if (reviewAnswer is not null) CompleteReview(new("cancel", [])); else Close(); };
        var confirm = new Button { Content = spec.PrimaryAction, Background = Brush(spec.Accent), BorderBrush = Brush(spec.Accent), Foreground = Brush("#1b2119"), Margin = new Thickness(0) };
        if (tools && reviewAnswer is null && outcomes.TryGetValue(spec.Title, out var outcome))
        {
            confirm.Content = dependencyPage ? (outcome.Status == "Installed" ? "Install / update" : "Retry") : outcome.Status;
            if (!string.IsNullOrEmpty(outcome.Detail)) footer.Children.Add(Text(outcome.Detail, 13, "#a9b09b", margin: new Thickness(0, 0, 0, 12)));
            if (Uri.TryCreate(outcome.Url, UriKind.Absolute, out var link) && link.Scheme == "https" && link.Host == "github.com")
            {
                var open = new Button { Content = "View submission", Margin = new Thickness(0, 0, 10, 0) };
                open.Click += (_, _) => System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(link.AbsoluteUri) { UseShellExecute = true });
                buttons.Children.Add(open);
            }
        }
        void UpdateConfirm() => confirm.IsEnabled = (!operating || reviewAnswer is not null) && (!checking.Contains(spec.Title) || spec.Title is "External dependencies" or "Update mods") &&
            (!dependencyPage || DependenciesActionable(spec)) &&
            (!tools || spec.Title != "Update mods" || inputs.Values.OfType<CheckBox>().Any(check => check.IsEnabled && check.IsChecked == true));
        foreach (var check in inputs.Values.OfType<CheckBox>())
        {
            check.Checked += (_, _) => UpdateConfirm();
            check.Unchecked += (_, _) => UpdateConfirm();
        }
        UpdateConfirm();
        confirm.Click += async (_, _) =>
        {
            if (reviewAnswer is not null) { var answer = ReadValues(); if (answer is not null) CompleteReview(new("confirm", answer)); return; }
            if (operating) return;
            if (dependencyPage && !DependenciesActionable(spec)) return;
            if (checking.Contains(spec.Title) && spec.Title is not ("External dependencies" or "Update mods")) return;
            if (tools && spec.Title == "GitHub") { await RefreshStatusAsync(true); return; }
            var values = ReadValues();
            if (values is null) return;
            if (preview)
            {
                feedback.Text = "Preview complete. No changes made.";
                feedback.Visibility = Visibility.Visible;
                feedback.BringIntoView();
                return;
            }
            if (tools && spec.Title == "Update mods" && !values.Values.OfType<bool>().Any(value => value))
            {
                feedback.Text = "Select at least one mod.";
                feedback.Visibility = Visibility.Visible;
                return;
            }
            if (tools && observeStatus) { await RunOperationAsync(ToolPages.Action(spec.Title), values); return; }
            Result = new DialogResult(tools ? ToolPages.Action(spec.Title) : "confirm", values);
            Close();
        };
        if (spec.SecondaryAction.Length > 0 && !dependencyPage && (!tools || reviewAnswer is not null)) buttons.Children.Add(cancel);
        foreach (var action in spec.Actions)
        {
            var button = new Button { Content = action.Label, Margin = new Thickness(0, 0, 10, 0) };
            button.Click += (_, _) => { var values = ReadValues(); if (values is null) return; if (reviewAnswer is not null) { CompleteReview(new(action.Id, values)); return; } Result = new DialogResult(action.Id, values); Close(); };
            buttons.Children.Add(button);
        }
        if (spec.Actions.Count == 0) buttons.Children.Add(confirm);
        footer.Children.Add(buttons);
        var footerBorder = new Border { Background = Background, Child = footer };
        Grid.SetRow(footerBorder, 2);
        body.Children.Add(footerBorder);
    }

    private async Task RefreshStatusAsync(bool signIn = false, string? title = null)
    {
        title ??= spec.Title;
        if (checking.Contains(title)) return;
        if (spec.Title == title) SaveDraft();
        checking.Add(title);
        UpdateRefreshButton();
        if (title == "Update mods") statusPages[title] = ToolPages.Updates();
        if (spec.Title == title) Render(title == "Update mods" ? ToolPages.Updates() : title == "External dependencies" ? spec : spec with { Items = [new("Status", "", signIn ? "Waiting for GitHub sign-in..." : "Checking...")] });
        DialogSpec result;
        try
        {
            result = title switch
            {
                "GitHub" => await ToolStatus.GitHubAsync(signIn, lifetime.Token),
                "Update mods" => await ToolStatus.ModsAsync(lifetime.Token, new Progress<DialogSpec>(page =>
                {
                    if (lifetime.IsCancellationRequested || !checking.Contains("Update mods")) return;
                    statusPages["Update mods"] = page;
                    if (!operating && reviewAnswer is null && spec.Title == "Update mods") { SaveDraft(); Render(page); }
                })),
                _ => await ToolStatus.DependenciesAsync(lifetime.Token)
            };
        }
        catch (Exception)
        {
            result = title == "External dependencies"
                ? ToolPages.Dependencies() with { Items = ToolPages.Dependencies().Items.Select(item => item with { Status = "Could not check status", State = "warning" }).ToList() }
                : (title == "GitHub" ? ToolPages.GitHub() : ToolPages.Updates("Unable to check - refresh")) with { Items = [new("Status", "", "Check unavailable \u00b7 try Refresh", "warning")] };
        }
        if (spec.Title == title) SaveDraft();
        checking.Remove(title);
        UpdateRefreshButton();
        if (lifetime.IsCancellationRequested) return;
        statusPages[title] = result;
        if (title == "GitHub")
            statusPages["Submit mission"] = ToolPages.Submission() with { Items = [.. ToolPages.Submission().Items, result.Items[0]] };
        if (!operating && reviewAnswer is null && (spec.Title == title || title == "GitHub" && spec.Title == "Submit mission") && statusPages.TryGetValue(spec.Title, out var current)) Render(current);
    }

    private Task<DialogResult> ReviewAsync(DialogSpec request)
    {
        reviewAnswer = new(TaskCreationOptions.RunContinuationsAsynchronously);
        Render(request);
        return reviewAnswer.Task;
    }

    private void CompleteReview(DialogResult answer)
    {
        var pending = reviewAnswer;
        reviewAnswer = null;
        // Disable the old form immediately: no repeated click may dispatch an action.
        if (operating) Render(new DialogSpec { Title = "Working", PrimaryAction = "Working...", SecondaryAction = "" });
        pending?.TrySetResult(answer);
    }

    private async Task RunOperationAsync(string action, Dictionary<string, object> values)
    {
        SaveDraft();
        var page = spec;
        if (action == "install")
        {
            if (!DependenciesActionable(page)) return;
            values["tools"] = !page.Items.Any(item => item.Title == "GC Workbench Tools" && item.State == "ready");
            values["git"] = !page.Items.Any(item => item.Title == "Git for Windows" && item.State == "ready");
        }
        operating = true;
        outcomes.Remove(page.Title);
        Render(page with { PrimaryAction = action switch { "submit" => "Preparing...", _ => "Updating..." }, SecondaryAction = "" });
        OperationResult result;
        try { result = await OperationRunner(action, values, ReviewAsync); }
        catch (Exception error) { result = new("Try again", error.Message); }
        finally { reviewAnswer = null; operating = false; }
        outcomes[page.Title] = result;
        Render(statusPages.GetValueOrDefault(page.Title, page));
        if (action is "update" or "install") await RefreshStatusAsync(false, page.Title);
    }

    private static bool DependenciesCurrent(DialogSpec page) =>
        new[] { "GC Workbench Tools", "Git for Windows" }.All(title => page.Items.Any(item => item.Title == title && item.State == "ready"));

    private static bool DependenciesActionable(DialogSpec page) => !DependenciesCurrent(page) &&
        new[] { "GC Workbench Tools", "Git for Windows" }.All(title => page.Items.Any(item => item.Title == title && item.State != "pending"));

    internal void UpdateDependencyProgress(DependencyProgress progress)
    {
        if (!operating || reviewAnswer is not null || spec.Title != "External dependencies") return;
        var title = progress.Dependency == "tools" ? "GC Workbench Tools" : "Git for Windows";
        var page = spec with { Items = spec.Items.Select(item => item.Title == title ? item with { Status = progress.Status, State = progress.State } : item).ToList() };
        statusPages[page.Title] = page with { PrimaryAction = "Install / update" };
        Render(page);
    }

    private FrameworkElement StatusCard(StatusItem item)
    {
        var panel = new StackPanel();
        var color = item.State switch { "ready" => "#a9b09b", "error" => "#e5a198", "warning" => "#d4b874", _ => "#a9b09b" };
        panel.Children.Add(Text(item.Title, 15, "#f0e9d5", bold: true));
        if (item.Description.Length > 0) panel.Children.Add(Text(item.Description, 13, "#a9b09b", margin: new Thickness(0, 5, 0, 0)));
        panel.Children.Add(Text(item.Status, 12, color, bold: true, margin: new Thickness(0, 8, 0, 0)));
        return Card(panel);
    }

    private FrameworkElement Field(InputField field)
    {
        var panel = new StackPanel();
        FrameworkElement input;
        if (field.Kind == "check")
        {
            var label = new StackPanel();
            label.Children.Add(Text(field.Label, 15, "#f0e9d5", bold: true));
            if (field.Help.Length > 0) label.Children.Add(Text(field.Help, 12, "#a9b09b", margin: new Thickness(0, 5, 0, 0)));
            input = new CheckBox { Content = label, IsChecked = field.Enabled && field.Checked, IsEnabled = field.Enabled, Opacity = field.Enabled ? 1 : 0.6, VerticalContentAlignment = VerticalAlignment.Center, Padding = new Thickness(10, 2, 0, 2) };
            panel.Children.Add(input);
        }
        else
        {
            panel.Children.Add(Text(field.Label + (field.Required ? " *" : ""), 14, "#f0e9d5", bold: true, margin: new Thickness(0, 0, 0, 8)));
            var text = new TextBox { Text = field.Value, Foreground = Brush("#f0e9d5"), CaretBrush = Brush("#f0e9d5"), Padding = new Thickness(12, 10, 12, 10), BorderBrush = Brush("#404735"), Background = Brush("#242b20"), BorderThickness = new Thickness(1), MaxLength = 8192 };
            text.IsReadOnly = field.ReadOnly;
            if (field.Kind is "multiline" or "filelist")
            {
                text.AcceptsReturn = true;
                text.TextWrapping = TextWrapping.Wrap;
                text.Height = field.Kind == "filelist" ? double.NaN : 106;
                if (field.Kind == "filelist")
                {
                    text.IsReadOnly = true;
                    text.Padding = new Thickness(10, 6, 10, 6);
                    text.MaxHeight = 240;
                    text.FontFamily = new FontFamily("Consolas");
                }
                text.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            }
            panel.Children.Add(text);
            input = text;
            if (field.Help.Length > 0) panel.Children.Add(Text(field.Help, 12, "#a9b09b", margin: new Thickness(0, 7, 0, 0)));
        }
        AutomationProperties.SetName(input, field.Label);
        AutomationProperties.SetHelpText(input, field.Help);
        inputs.Add(field.Id, input);
        var error = Text("", 12, "#e5a198", margin: new Thickness(0, 8, 0, 0));
        error.Visibility = Visibility.Collapsed;
        errors.Add(field.Id, error);
        panel.Children.Add(error);
        return field.Kind == "check" ? Card(panel) : new Border { Child = panel, Margin = new Thickness(0, 0, 0, field.Kind == "filelist" ? 12 : 20) };
    }

    private void SaveDraft()
    {
        if (checking.Contains(spec.Title) && spec.Title != "Update mods") return;
        if (!drafts.TryGetValue(spec.Title, out var draft)) drafts[spec.Title] = draft = [];
        foreach (var pair in inputs.Where(pair => pair.Value.IsEnabled))
            draft[pair.Key] = pair.Value is CheckBox check ? (object)(check.IsChecked == true) : ((TextBox)pair.Value).Text;
    }

    private Dictionary<string, object>? ReadValues()
    {
        var values = new Dictionary<string, object>();
        FrameworkElement? firstInvalid = null;
        foreach (var field in spec.Fields)
        {
            var control = inputs[field.Id];
            object value = control is CheckBox check ? check.IsEnabled && check.IsChecked == true : ((TextBox)control).Text.Trim();
            values[field.Id] = value;
            bool missing = field.Required && (value is bool flag ? !flag : string.IsNullOrWhiteSpace((string)value));
            errors[field.Id].Text = missing ? "Please complete this field." : "";
            errors[field.Id].Visibility = missing ? Visibility.Visible : Visibility.Collapsed;
            if (missing) firstInvalid ??= control;
        }
        if (firstInvalid is null) return values;
        firstInvalid.Focus();
        firstInvalid.BringIntoView();
        return null;
    }

    private static Border Card(UIElement content) => new()
    {
        Child = content, Padding = new Thickness(0, 16, 0, 16),
        Background = Brushes.Transparent, BorderBrush = Brush("#404735"), BorderThickness = new Thickness(0, 0, 0, 1)
    };

    private static SolidColorBrush Brush(string hex) => new((Color)ColorConverter.ConvertFromString(hex));
    private static TextBlock Text(string text, double size, string color, bool bold = false, Thickness margin = default) => new()
    {
        Text = text, FontSize = size, Foreground = Brush(color), TextWrapping = TextWrapping.Wrap,
        FontWeight = bold ? FontWeights.SemiBold : FontWeights.Normal, Margin = margin
    };

    private const string Styles = """
        <ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
          <Style TargetType="ScrollBar">
            <Setter Property="Width" Value="10"/>
            <Setter Property="Background" Value="#191e18"/>
            <Setter Property="Template"><Setter.Value><ControlTemplate TargetType="ScrollBar">
              <Grid Background="{TemplateBinding Background}">
                <Track x:Name="PART_Track" IsDirectionReversed="True" Orientation="{TemplateBinding Orientation}">
                  <Track.DecreaseRepeatButton><RepeatButton Command="ScrollBar.PageUpCommand" Opacity="0" Focusable="False"/></Track.DecreaseRepeatButton>
                  <Track.Thumb><Thumb><Thumb.Template><ControlTemplate TargetType="Thumb"><Border Background="#404735" CornerRadius="3" Margin="2"/></ControlTemplate></Thumb.Template></Thumb></Track.Thumb>
                  <Track.IncreaseRepeatButton><RepeatButton Command="ScrollBar.PageDownCommand" Opacity="0" Focusable="False"/></Track.IncreaseRepeatButton>
                </Track>
              </Grid>
            </ControlTemplate></Setter.Value></Setter>
          </Style>
          <Style TargetType="TextBox">
            <Setter Property="Template"><Setter.Value><ControlTemplate TargetType="TextBox">
              <Border x:Name="Frame" CornerRadius="3" Background="{TemplateBinding Background}" BorderBrush="{TemplateBinding BorderBrush}" BorderThickness="{TemplateBinding BorderThickness}" Padding="{TemplateBinding Padding}">
                <ScrollViewer x:Name="PART_ContentHost"/>
              </Border>
              <ControlTemplate.Triggers><Trigger Property="IsKeyboardFocused" Value="True"><Setter TargetName="Frame" Property="BorderBrush" Value="#d4b874"/></Trigger></ControlTemplate.Triggers>
            </ControlTemplate></Setter.Value></Setter>
          </Style>
          <Style TargetType="CheckBox">
            <Setter Property="Cursor" Value="Hand"/>
            <Setter Property="Template"><Setter.Value><ControlTemplate TargetType="CheckBox">
              <Grid>
                <Grid.ColumnDefinitions><ColumnDefinition Width="22"/><ColumnDefinition Width="*"/></Grid.ColumnDefinitions>
                <Border x:Name="Box" Width="20" Height="20" VerticalAlignment="Center" Background="#242b20" BorderBrush="#a9b09b" BorderThickness="1" CornerRadius="4">
                  <Path x:Name="Tick" Visibility="Collapsed" Data="M 3,9 L 7,13 L 15,4" Stroke="#1b2119" StrokeThickness="2"/>
                </Border>
                <ContentPresenter Grid.Column="1" Margin="12,0,0,0" VerticalAlignment="Center" RecognizesAccessKey="True"/>
              </Grid>
              <ControlTemplate.Triggers>
                <Trigger Property="IsChecked" Value="True"><Setter TargetName="Tick" Property="Visibility" Value="Visible"/><Setter TargetName="Box" Property="Background" Value="{DynamicResource GcAccent}"/><Setter TargetName="Box" Property="BorderBrush" Value="#a9b09b"/></Trigger>
                <Trigger Property="IsKeyboardFocused" Value="True"><Setter TargetName="Box" Property="BorderBrush" Value="#d4b874"/><Setter TargetName="Box" Property="BorderThickness" Value="2"/></Trigger>
              </ControlTemplate.Triggers>
            </ControlTemplate></Setter.Value></Setter>
          </Style>
          <Style TargetType="Button">
            <Setter Property="Foreground" Value="#f0e9d5"/>
            <Setter Property="Padding" Value="18,11"/>
            <Setter Property="MinHeight" Value="42"/>
            <Setter Property="Background" Value="#242b20"/>
            <Setter Property="BorderBrush" Value="#404735"/>
            <Setter Property="BorderThickness" Value="1"/>
            <Setter Property="FontWeight" Value="SemiBold"/>
            <Setter Property="Cursor" Value="Hand"/>
            <Setter Property="Template">
              <Setter.Value><ControlTemplate TargetType="Button">
                <Border x:Name="Frame" Background="{TemplateBinding Background}" BorderBrush="{TemplateBinding BorderBrush}" BorderThickness="{TemplateBinding BorderThickness}" CornerRadius="3" Padding="{TemplateBinding Padding}">
                  <ContentPresenter HorizontalAlignment="{TemplateBinding HorizontalContentAlignment}" VerticalAlignment="Center" RecognizesAccessKey="True"/>
                </Border>
                <ControlTemplate.Triggers>
                  <Trigger Property="IsMouseOver" Value="True"><Setter TargetName="Frame" Property="Opacity" Value="0.8"/></Trigger>
                  <Trigger Property="IsKeyboardFocused" Value="True"><Setter TargetName="Frame" Property="BorderBrush" Value="#d4b874"/></Trigger>
                  <Trigger Property="IsEnabled" Value="False"><Setter TargetName="Frame" Property="Opacity" Value="0.45"/></Trigger>
                </ControlTemplate.Triggers>
              </ControlTemplate></Setter.Value>
            </Setter>
          </Style>
        </ResourceDictionary>
        """;
}
