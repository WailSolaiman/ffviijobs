// Exercise the installed Configure Mod view model privately. Does not create
// an application window, import a mod, save settings, or start FF7.
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
class ConfigureTest {
 static int checks,reads;static string program,fixture;static object archive;static MethodInfo getBytes;
 static void Check(bool pass,string message){++checks;if(!pass)throw new Exception(message);}
 static object Get(object v,string n){return v.GetType().GetProperty(n).GetValue(v,null);}
 static void Set(object v,string n,object value){v.GetType().GetProperty(n).SetValue(v,value,null);}
 static void Private(object v,string n,object value){v.GetType().GetField(n,BindingFlags.Instance|BindingFlags.NonPublic).SetValue(v,value);}
 static object Construct(Type t,string path){var ctor=t.GetConstructors()[0];var pars=ctor.GetParameters();object[] vals=new object[pars.Length];for(int i=0;i<vals.Length;++i)vals[i]=i==0?(object)path:pars[i].HasDefaultValue?pars[i].DefaultValue:pars[i].ParameterType==typeof(bool)?(object)false:null;return ctor.Invoke(vals);}
 static string Preview(string path){
  Check(!string.IsNullOrWhiteSpace(path),"every selection has a preview");
  Check(path.StartsWith("previews\\",StringComparison.Ordinal),"preview is inside this mod");
  ++reads;byte[] bytes=(byte[])getBytes.Invoke(archive,new object[]{path});
  Check(bytes!=null && bytes.Length>32,"preview exists in the IRO");
  Check(bytes[0]==137 && bytes[1]==80 && bytes[2]==78 && bytes[3]==71,"PNG signature");
  string local=Path.Combine(fixture,path);Directory.CreateDirectory(Path.GetDirectoryName(local));
  if(!File.Exists(local))File.WriteAllBytes(local,bytes);return local;
 }
 [STAThread]static int Main(string[] args){try{
  program=Path.GetFullPath(args[0]);string iro=Path.GetFullPath(args[1]),manifest=Path.GetFullPath(args[2]);fixture=Path.GetFullPath(args[3]);Directory.CreateDirectory(fixture);Directory.SetCurrentDirectory(fixture);
  AppDomain.CurrentDomain.AssemblyResolve+=delegate(object s,ResolveEventArgs e){string path=Path.Combine(program,new AssemblyName(e.Name).Name+".dll");return File.Exists(path)?Assembly.LoadFrom(path):null;};
  var wrapper=Assembly.LoadFrom(Path.Combine(program,"AppWrapper.dll"));var ui=Assembly.LoadFrom(Path.Combine(program,"7th Heaven.dll"));
  archive=Construct(wrapper.GetType("AppWrapper.IrosArc"),iro);getBytes=archive.GetType().GetMethod("GetBytes");
  object info=Construct(wrapper.GetType("AppWrapper.ModInfo"),manifest);
  var options=(IList)Get(info,"Options");Check(options.Count==10,"nine characters and one accessory reference");
  var vmType=ui.GetType("AppUI.ViewModels.ConfigureModViewModel");object vm=Activator.CreateInstance(vmType);
  Type constraint=ui.GetType("AppUI.Classes.Constraint");var constraints=(IList)Activator.CreateInstance(typeof(List<>).MakeGenericType(constraint));
  foreach(object option in options){object c=Activator.CreateInstance(constraint);Set(c,"Setting",Get(option,"ID"));constraints.Add(c);}
  Private(vm,"_constraints",constraints);Private(vm,"_values",new Dictionary<string,int>());
  Private(vm,"_imageReader",new Func<string,string>(Preview));Private(vm,"_iroPath",iro);
  var optionVm=ui.GetType("AppUI.ViewModels.ConfigOptionViewModel");
  Type bitmap=Assembly.Load("PresentationCore").GetType("System.Windows.Media.Imaging.BitmapImage");
  var decoded=new HashSet<string>();int classes=0,accessories=0;
  for(int c=0;c<options.Count;++c){
   object option=options[c];string id=(string)Get(option,"ID");
   Check(c<9?id.StartsWith("job_v070_"):id=="accessory_reference_v071","existing class settings stay compatible");
   object item=Activator.CreateInstance(optionVm,new object[]{option});Set(vm,"SelectedOption",item);
   Check((string)Get(vm,"Description")== (string)Get(option,"Description"),"character/reference instructions displayed");
   var values=(IList)Get(option,"Values");Check(values.Count==(c<9?25:32),"all guide choices available");
   for(int j=0;j<values.Count;++j){
    Set(vm,"DropdownSelectedIndex",j);object value=values[j];
    Check((string)Get(vm,"SelectedDropdownText")== (string)Get(value,"Name"),"selected class/item label");
    Uri actual=(Uri)Get(vm,"ImageOptionSource");string expected=Path.Combine(fixture,(string)Get(value,"PreviewFile"));
    Check(actual!=null && actual.LocalPath.Equals(expected,StringComparison.OrdinalIgnoreCase),"selected guide image updates immediately");
    if(decoded.Add(expected)){
     object image=Activator.CreateInstance(bitmap,new object[]{actual});
     Check((int)Get(image,"PixelWidth")==1280,"WPF decodes full-width guide image");
     Check((int)Get(image,"PixelHeight")>0 && (int)Get(image,"PixelHeight")<=1300,"guide fits default preview proportions");
    }
    if(c<9)++classes;else ++accessories;
   }
  }
  Check(classes==225 && accessories==32,"all 257 selections exercised");Check(decoded.Count==57,"25 class and 32 accessory previews decoded");
  Console.WriteLine("PASS "+checks+" installed Configure Mod view-model checks; "+classes+" class selections, "+accessories+" accessory selections, "+decoded.Count+" decoded previews.");return 0;
 }catch(Exception e){Console.WriteLine(e);return 1;}}
}
