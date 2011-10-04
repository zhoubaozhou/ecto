#include <gtest/gtest.h>
#include <ecto/ecto.hpp>
#include <ecto/except.hpp>
#include <ecto/plasm.hpp>
#include <ecto/schedulers/threadpool.hpp>

#define STRINGDIDLY(A) std::string(#A)

using namespace ecto;

struct InConstructorExcept
{
  InConstructorExcept() {
    throw std::logic_error("no.... I do not want to live.");
  }
};

struct ExceptionalModule1
{
  static void
  declare_params(tendrils& p)
  {
    p.declare<double> ("d");
    p.declare<float> ("f").set_default_val(p.get<float> ("d"));
  }
};

struct ExceptionUnknownException
{
  static void
  declare_params(tendrils& p)
  {
    p.declare<double> ("d");
  }
  static void
  declare_io(const tendrils& p, tendrils& in, tendrils& out)
  {
    in.declare<double> ("d");
    throw "A string";
  }
};

struct NotExist
{
  static void
  declare_params(tendrils& p)
  {
    p.declare<int> ("a");
  }
  static void
  declare_io(const tendrils& p, tendrils& in, tendrils& out)
  {
    in.declare<double> ("d");
    in.declare<ExceptionalModule1> ("c");
    in.declare<std::string> ("e");
    out.declare<std::string> ("a");
  }

  int
  process(const tendrils& in, const tendrils& out)
  {
    in.get<double> ("a");
    return 0;
  }
};

struct WrongType
{
  static void
  declare_io(const tendrils& p, tendrils& in, tendrils& out)
  {
    in.declare<double> ("d");
  }
  int
  process(const tendrils& in, const tendrils& out)
  {
    in.get<int> ("d");
    return 0;
  }
};

struct ParameterCBExcept
{
  static void
  declare_params(tendrils& p)
  {
    p.declare<double> ("x");
  }
  void xcb(double x)
  {
    std::cout << "called back***" << std::endl;
    throw std::runtime_error("I'm a bad callback, and I like it that way.");
  }
  void
  configure(const tendrils& p,const tendrils& in, const tendrils& out)
  {
    std::cout << "configurated ***" << std::endl;
    spore<double> x = p["x"];
    x.set_callback(boost::bind(&ParameterCBExcept::xcb,this,_1));
  }
};

struct ProcessException
{
  int
  process(const tendrils& in, const tendrils& out)
  {
    throw std::logic_error("A standard exception");
    return ecto::OK;
  }
};

TEST(Exceptions, ExceptionalModules)
{
  try
  {
    cell* p = new cell_<ExceptionalModule1>;
  } catch (except::EctoException& e)
  {
    std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
  }
  EXPECT_THROW(new cell_<ExceptionalModule1>, ecto::except::EctoException);
}
TEST(Exceptions, ExceptionUnknownException)
{
  try
  {
    new cell_<ExceptionUnknownException>;
  } catch (except::EctoException& e)
  {
    std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
  }
  EXPECT_THROW(new cell_<ExceptionUnknownException>, ecto::except::EctoException);
}

#define MEH(x, y) x

TEST(Exceptions, ProcessException)
{
  std::string stre("Original Exception: std::logic_error\n"
    "  What   : A standard exception\n"
    "  Module : ProcessException\n"
    "  Function: process");
  cell::ptr m(new cell_<ProcessException>);
  EXPECT_THROW(
      try
      {
        m->process();
      }
      catch (except::EctoException& e)
      {
        std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
        std::cout << diagnostic_information(e) << "\n";
        /*
        if(stre != e.msg_)
        {
          throw std::runtime_error("Got :" + e.msg_ +"\nExpected :" +stre);
        }
        */
        throw;
      }
      ,
      ecto::except::EctoException);
}

TEST(Exceptions, NotExist)
{
  std::string
      stre(
           "'a' does not exist in this tendrils object. Possible keys are:  'c':type(ExceptionalModule1) 'd':type(double) 'e':type(std::string)\n"
             "  Hint   : 'a' does exist in parameters (type == int) outputs (type == std::string)\n"
             "  Module : NotExist\n"
             "  Function: process");

  cell::ptr m(new cell_<NotExist>);
  try
    {
      m->process();
    }
  catch (except::NonExistant& e)
    {
      std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
      //EXPECT_EQ(stre, e.msg_);
    }
}

TEST(Exceptions, WrongType)
{
  std::string stre("double is not a int\n"
"  Hint : 'd' is of type double\n"
"  Module : WrongType\n"
"  Function: process");
  cell::ptr m(new cell_<WrongType>);
  bool threw = false;
  try
    {
      m->process();
    }
  catch (except::EctoException& e)
    {
      std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
      //      EXPECT_EQ(stre, e.msg_);
      threw = true;
    }
  EXPECT_TRUE(threw);
}

TEST(Exceptions, WrongType_sched)
{
  std::string stre("double is not a int\n"
"  Hint : 'd' is of type double\n"
"  Module : WrongType\n"
"  Function: process");
  cell::ptr m(new cell_<WrongType>);
  plasm::ptr p(new plasm);
  p->insert(m);
  schedulers::threadpool sched(p);
  bool threw = false;
  try
    {
      sched.execute(8,1);
    }
  catch (except::TypeMismatch& e)
    {
      std::cout << "Good, threw an exception:\n" << e.what() << std::endl;
      //      EXPECT_EQ(stre, e.msg_);
      threw = true;
    }
  EXPECT_TRUE(threw);
}

TEST(Exceptions, ParameterCBExcept_sched)
{
  cell::ptr m(new cell_<ParameterCBExcept>);
  m->parameters["x"] << 5.1;
  m->parameters["x"]->dirty(true);
  plasm::ptr p(new plasm);
  p->insert(m);
  schedulers::threadpool sched(p);
  EXPECT_THROW(
      try
      {
        sched.execute(8,1);
      }
      catch (except::EctoException& e)
      {
        std::cout << "Good, threw an exception:\n" << ecto::except::diagnostic_string(e) << std::endl;
        throw;
      }
      ,
      ecto::except::EctoException);
}

TEST(Exceptions, ConstructorExcept)
{
  cell::ptr m(new cell_<InConstructorExcept>);
  plasm::ptr p(new plasm);
  p->insert(m);
  schedulers::threadpool sched(p);
  try
    {
      sched.execute(8,1);
      FAIL();
    }
  catch (except::EctoException& e)
    {
      std::cout << "Good, threw an exception:\n"
                << ecto::except::diagnostic_string(e)
                << std::endl;
      const std::string* what = boost::get_error_info<ecto::except::what>(e);
      EXPECT_TRUE(what);
      EXPECT_EQ(*what, std::string("no.... I do not want to live."));
    }
}
