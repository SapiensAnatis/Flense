#include "pch.h"

#include "FilesystemChangesView.xaml.h"
#if __has_include("FilesystemChangesView.g.cpp")
#include "FilesystemChangesView.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::Flense::implementation
{
    DependencyProperty FilesystemChangesView::s_itemsSourceProperty{nullptr};
    DependencyProperty FilesystemChangesView::s_searchQueryProperty{nullptr};
    DependencyProperty FilesystemChangesView::s_changeVisibilityProperty{nullptr};

    FilesystemChangesView::FilesystemChangesView()
    {
        InitializeProperties();
        InitializeComponent();
    }

    void FilesystemChangesView::InitializeProperties()
    {
        // Registered on first construction rather than at namespace scope: static initialisers run
        // when the module loads, before the XAML runtime is up.
        if (!s_itemsSourceProperty)
        {
            s_itemsSourceProperty = DependencyProperty::Register(
                L"ItemsSource", winrt::xaml_typename<IObservableVector<winrt::Flense::FilesystemTreeNode>>(),
                winrt::xaml_typename<winrt::Flense::FilesystemChangesView>(),
                PropertyMetadata{nullptr, PropertyChangedCallback{&FilesystemChangesView::OnItemsSourceChanged}});
        }

        if (!s_searchQueryProperty)
        {
            s_searchQueryProperty = DependencyProperty::Register(
                L"SearchQuery", winrt::xaml_typename<hstring>(),
                winrt::xaml_typename<winrt::Flense::FilesystemChangesView>(),
                PropertyMetadata{winrt::box_value(hstring{}),
                                 PropertyChangedCallback{&FilesystemChangesView::OnSearchQueryChanged}});
        }

        if (!s_changeVisibilityProperty)
        {
            static constexpr winrt::Flense::FilesystemChangeVisibility DefaultChangeVisibility{true, true, true, true};

            s_changeVisibilityProperty = DependencyProperty::Register(
                L"ChangeVisibility", winrt::xaml_typename<winrt::Flense::FilesystemChangeVisibility>(),
                winrt::xaml_typename<winrt::Flense::FilesystemChangesView>(),
                PropertyMetadata{winrt::box_value(DefaultChangeVisibility),
                                 PropertyChangedCallback{&FilesystemChangesView::OnChangeVisibilityChanged}});
        }
    }

    winrt::Flense::FilesystemChangesViewModel FilesystemChangesView::ViewModel()
    {
        return m_viewModel;
    }

    DependencyProperty FilesystemChangesView::ItemsSourceProperty()
    {
        return s_itemsSourceProperty;
    }

    IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemChangesView::ItemsSource()
    {
        return GetValue(ItemsSourceProperty()).try_as<IObservableVector<winrt::Flense::FilesystemTreeNode>>();
    }

    void FilesystemChangesView::ItemsSource(const IObservableVector<winrt::Flense::FilesystemTreeNode>& value)
    {
        SetValue(ItemsSourceProperty(), value);
    }

    void FilesystemChangesView::OnItemsSourceChanged(const DependencyObject& sender,
                                                     const DependencyPropertyChangedEventArgs& args)
    {
        const auto view = winrt::get_self<FilesystemChangesView>(sender.as<winrt::Flense::FilesystemChangesView>());
        const auto nodes = args.NewValue().try_as<IObservableVector<winrt::Flense::FilesystemTreeNode>>();

        view->m_viewModel.Nodes(nodes);
    }

    DependencyProperty FilesystemChangesView::SearchQueryProperty()
    {
        return s_searchQueryProperty;
    }

    hstring FilesystemChangesView::SearchQuery()
    {
        return winrt::unbox_value<hstring>(GetValue(SearchQueryProperty()));
    }

    void FilesystemChangesView::SearchQuery(const hstring& value)
    {
        SetValue(SearchQueryProperty(), winrt::box_value(value));
    }

    void FilesystemChangesView::OnSearchQueryChanged(const DependencyObject& sender,
                                                     const DependencyPropertyChangedEventArgs& args)
    {
        const auto view = winrt::get_self<FilesystemChangesView>(sender.as<winrt::Flense::FilesystemChangesView>());
        view->m_viewModel.SearchQuery(winrt::unbox_value<hstring>(args.NewValue()));
    }

    DependencyProperty FilesystemChangesView::ChangeVisibilityProperty()
    {
        return s_changeVisibilityProperty;
    }

    winrt::Flense::FilesystemChangeVisibility FilesystemChangesView::ChangeVisibility()
    {
        return winrt::unbox_value<winrt::Flense::FilesystemChangeVisibility>(GetValue(ChangeVisibilityProperty()));
    }

    void FilesystemChangesView::ChangeVisibility(const winrt::Flense::FilesystemChangeVisibility& value)
    {
        SetValue(ChangeVisibilityProperty(), winrt::box_value(value));
    }

    void FilesystemChangesView::OnChangeVisibilityChanged(const DependencyObject& sender,
                                                           const DependencyPropertyChangedEventArgs& args)
    {
        const auto view = winrt::get_self<FilesystemChangesView>(sender.as<winrt::Flense::FilesystemChangesView>());
        view->m_viewModel.ChangeVisibility(
            winrt::unbox_value<winrt::Flense::FilesystemChangeVisibility>(args.NewValue()));
    }
} // namespace winrt::Flense::implementation
